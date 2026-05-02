import socket
import time
import unittest
import threading

class TestRedisServer(unittest.TestCase):
    def setUp(self):
        self.host = "127.0.0.1"
        self.port = 4242

    def send_query(self, query):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((self.host, self.port))
            s.sendall(f"{query}\n".encode())
            data = s.recv(1024)
            return data.decode()

    def test_set_get(self):
        self.assertEqual(self.send_query("SET key1 value1"), "+OK\r\n")
        self.assertEqual(self.send_query("GET key1"), "+value1\r\n")

    def test_exists_del(self):
        self.send_query("SET key2 value2")
        self.assertEqual(self.send_query("EXISTS key2"), ":1\r\n")
        self.assertEqual(self.send_query("DEL key2"), "+OK\r\n")
        self.assertEqual(self.send_query("EXISTS key2"), ":0\r\n")

    def test_expired_key(self):
        self.send_query("SET temp_key val")
        self.assertEqual(self.send_query("EXPIRES temp_key 1"), "+OK\r\n")
        self.assertEqual(self.send_query("GET temp_key"), "+val\r\n")
        
        time.sleep(1.2)
        self.assertEqual(self.send_query("GET temp_key"), "$-1\r\n")

    def test_unknown_command(self):
        response = self.send_query("ABOBA 123")
        self.assertTrue(response.startswith("-ERR"))

    def test_dirty_input(self):
        self.assertEqual(self.send_query("SET    key_space       value_space"), "+OK\r\n")
        self.assertEqual(self.send_query("GET    key_space"), "+value_space\r\n")
        self.assertTrue(self.send_query("").startswith("-ERR"))

    def test_large_value(self):
        large_val = "A" * 1000
        self.assertEqual(self.send_query(f"SET big_key {large_val}"), "+OK\r\n")
        self.assertEqual(self.send_query("GET big_key"), f"+{large_val}\r\n")

    def test_overwrite_and_ttl_reset(self):
        self.send_query("SET key_to_overwrite val1")
        self.send_query("EXPIRES key_to_overwrite 1")
        self.assertEqual(self.send_query("SET key_to_overwrite val2"), "+OK\r\n")
        self.assertEqual(self.send_query("GET key_to_overwrite"), "+val2\r\n")
        time.sleep(1.2)
        self.assertEqual(self.send_query("GET key_to_overwrite"), "+val2\r\n")

    def test_multiple_clients(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s1, \
            socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s2:
            s1.connect((self.host, self.port))
            s2.connect((self.host, self.port))
            
            s1.sendall(b"SET multi key1\n")
            self.assertEqual(s1.recv(1024).decode(), "+OK\r\n")
            
            s2.sendall(b"GET multi\n")
            self.assertEqual(s2.recv(1024).decode(), "+key1\r\n")

    def test_stress_deadlock_passive_expiry(self):
        num_threads = 16
        ops_per_thread = 100
        errors = []

        def worker():
            try:
                thread_id = threading.get_ident()
                key = f"stress_key_{thread_id}"
                
                for i in range(ops_per_thread):
                    self.send_query(f"SET {key} val_{i}")
                    self.send_query(f"EXPIRES {key} 0")
                    
                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                        s.settimeout(2.0)
                        s.connect((self.host, self.port))
                        s.sendall(f"GET {key}\n".encode())
                        
                        resp = s.recv(1024).decode()
                        if not resp:
                            errors.append(f"Thread {thread_id}: Empty response (possible deadlock)")
            except socket.timeout:
                errors.append(f"Thread {thread_id}: Connection timed out (DEADLOCK!)")
            except Exception as e:
                errors.append(f"Thread {thread_id}: Exception {e}")

        threads = [threading.Thread(target=worker) for _ in range(num_threads)]
        
        for t in threads: t.start()
        for t in threads: t.join()

        self.assertEqual(len(errors), 0, f"Deadlock test failed with errors: {errors}")

if __name__ == "__main__":
    unittest.main()