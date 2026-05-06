import subprocess
import socket
import time
import unittest
import os
import signal

SERVER_PATH = "./build/mredis" 
AOF_PATH = "./aof_save"

class TestRedisPersistence(unittest.TestCase):
    
    def setUp(self):
        if os.path.exists(AOF_PATH):
            os.remove(AOF_PATH)
        self.server_proc = None
        self.port = 4242

    def tearDown(self):
        self.stop_server()
        if os.path.exists(AOF_PATH):
            os.remove(AOF_PATH)

    def start_server(self):
        self.server_proc = subprocess.Popen(
            [SERVER_PATH],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn=os.setsid
        )

        time.sleep(0.2)

    def stop_server(self):
        if self.server_proc and self.server_proc.poll() is None:
            try:
                os.killpg(os.getpgid(self.server_proc.pid), signal.SIGINT)
                self.server_proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                os.killpg(os.getpgid(self.server_proc.pid), signal.SIGKILL)
            self.server_proc = None

    def send_query(self, query):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(("127.0.0.1", self.port))
            s.sendall(f"{query}\n".encode())
            return s.recv(1024).decode()

    def test_basic_recovery(self):
        self.start_server()
        
        self.assertEqual(self.send_query("SET mykey value123"), "+OK\r\n")
        self.assertEqual(self.send_query("GET mykey"), "+value123\r\n")
        
        self.stop_server() 
        self.start_server() 
        
        self.assertEqual(self.send_query("GET mykey"), "+value123\r\n")

    def test_expired_keys_not_recovered(self):
        self.start_server()
        
        self.send_query("SET tempkey temporary")
        self.send_query("EXPIRES tempkey 1")
        
        time.sleep(1.2)
        
        self.stop_server()
        self.start_server()
        
        self.assertEqual(self.send_query("GET tempkey"), "$-1\r\n")

    def test_ttl_overwrite_regression(self):
        self.start_server()
        
        self.send_query("SET session token_old")
        self.send_query("EXPIRES session 1")      # Первоначальный короткий TTL (1 сек)
        self.send_query("SET session token_new")  # Перезапись ключа (сбрасывает TTL в -1 или дефолт)
        
        time.sleep(1.2)
        
        self.stop_server()
        self.start_server()
        
        self.assertEqual(self.send_query("GET session"), "+token_new\r\n")

    def test_only_mutating_commands_persisted(self):
        self.start_server()
        
        self.send_query("SET clean_key 100")
        self.send_query("GET clean_key") # Чтение
        self.send_query("EXISTS clean_key") # Чтение
        self.send_query("DEL clean_key")
        
        self.stop_server()
        
        self.assertTrue(os.path.exists(AOF_PATH), "AOF file should exist")
        
        with open(AOF_PATH, "r") as f:
            lines = [line.strip() for line in f.readlines() if line.strip()]
            
        for line in lines:
            self.assertFalse(line.startswith("GET"), f"AOF contains read-only command: {line}")
            self.assertFalse(line.startswith("EXISTS"), f"AOF contains read-only command: {line}")
            
        self.assertEqual(len(lines), 2)
        self.assertTrue(lines[0].startswith("SET"))
        self.assertTrue(lines[1].startswith("DEL"))

if __name__ == "__main__":
    unittest.main()