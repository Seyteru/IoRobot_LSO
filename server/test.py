#!/usr/bin/env python3
import socket
import json
import threading
import time
import random
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

class IoRobotClient:
    def __init__(self, host='localhost', port=5555, client_id=None):
        self.host = host
        self.port = port
        self.client_id = client_id or f"client_{random.randint(1000, 9999)}"
        self.socket = None
        self.responses = []
        self.errors = []
        
    def connect(self):
        """Connette al server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"[{self.client_id}] Connected to server {self.host}:{self.port}")
            return True
        except Exception as e:
            self.errors.append(f"Connection failed: {e}")
            print(f"[{self.client_id}] Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnette dal server"""
        if self.socket:
            try:
                self.socket.close()
                print(f"[{self.client_id}] Disconnected from server")
            except Exception as e:
                print(f"[{self.client_id}] Error during disconnection: {e}")
    
    def send_message(self, message):
        """Invia un messaggio al server"""
        try:
            if isinstance(message, str):
                self.socket.send(message.encode() + b'\n')
            else:
                self.socket.send(str(message).encode() + b'\n')
            return True
        except Exception as e:
            self.errors.append(f"Send failed: {e}")
            print(f"[{self.client_id}] Send error: {e}")
            return False
    
    def receive_all_pending_messages(self, timeout=5):
        """Riceve tutti i messaggi in coda dal server"""
        messages = []
        self.socket.settimeout(0.1)
        
        while True:
            try:
                data = self.socket.recv(4096)
                if data:
                    decoded = data.decode().strip()
                    lines = decoded.split('\n')
                    for line in lines:
                        line = line.strip()
                        if line:
                            messages.append(line)
                            self.responses.append(line)
                else:
                    break
            except socket.timeout:
                break
            except Exception as e:
                print(f"[{self.client_id}] Error in receive_all_pending: {e}")
                break
        
        self.socket.settimeout(timeout)
        return messages

    def receive_message(self, timeout=30):
        """Riceve un messaggio dal server"""
        try:
            self.socket.settimeout(timeout)
            data = self.socket.recv(4096)
            if data:
                decoded = data.decode().strip()
                lines = decoded.split('\n')
                for line in lines:
                    line = line.strip()
                    if line:
                        self.responses.append(line)
                        return line
            return None
        except socket.timeout:
            self.errors.append("Timeout receiving message")
            print(f"[{self.client_id}] Timeout receiving message")
            return None
        except Exception as e:
            self.errors.append(f"Receive failed: {e}")
            print(f"[{self.client_id}] Receive error: {e}")
            return None
    
    def parse_json_message(self, message):
        """Parse del messaggio JSON dal server"""
        try:
            clean_message = message.strip()
            print(f"[{self.client_id}] Raw message: {repr(message)}")
            print(f"[{self.client_id}] Clean message: {clean_message}")

            parsed = json.loads(clean_message)
            return parsed
        except json.JSONDecodeError as e:
            self.errors.append(f"JSON parsing failed: {e}")
            print(f"[{self.client_id}] JSON parsing error: {e}")
            print(f"[{self.client_id}] Problematic message: {repr(message)}")
            
            try:
                start = message.find('{')
                end = message.rfind('}') + 1
                if start >= 0 and end > start:
                    json_part = message[start:end]
                    print(f"[{self.client_id}] Trying with JSON part: {json_part}")
                    return json.loads(json_part)
            except:
                pass
            
            return None
    
    def complete_personality_assessment(self, response_pattern="random"):
        """Completa l'assessment della personalità"""
        print(f"[{self.client_id}] Starting personality assessment...")
        
        question_count = 0
        max_questions = 10
        
        while question_count < max_questions:
            message = self.receive_message()
            if not message:
                print(f"[{self.client_id}] No message received, terminating")
                break
                
            parsed = self.parse_json_message(message)
            if not parsed:
                print(f"[{self.client_id}] Parsing failed, trying to continue...")
                continue
                
            msg_type = parsed.get('type', '')
            print(f"[{self.client_id}] Received type: {msg_type}")
            
            if msg_type in ['ask', 'gpt_ask']:
                question = parsed.get('question', '')
                q_num = parsed.get('questionNum', question_count + 1)
                
                print(f"[{self.client_id}] Q{q_num}: {question[:100]}...")
                
                if response_pattern == "random":
                    response = random.randint(1, 7)
                elif response_pattern == "introvert":
                    if q_num in [1, 3, 5, 7, 9]:
                        response = random.randint(1, 3)
                    else:
                        response = random.randint(5, 7)
                elif response_pattern == "extrovert":
                    if q_num in [1, 3, 5, 7, 9]:
                        response = random.randint(5, 7)
                    else:
                        response = random.randint(1, 3)
                elif response_pattern == "neutral":
                    response = random.randint(3, 5)
                else:
                    response = random.randint(1, 7)
                
                if not self.send_message(str(response)):
                    break
                    
                print(f"[{self.client_id}] Answered: {response}")
                question_count += 1
                time.sleep(random.uniform(0.1, 0.5))
                
            elif msg_type == 'result':
                personality = parsed.get('personality', {})
                if isinstance(personality, dict):
                    extroversion = personality.get('extroversion', 0)
                    style = personality.get('style', 'unknown')
                    instructions = personality.get('instructions', [])
                else:
                    extroversion = 0
                    style = str(personality)
                    instructions = []
                
                print(f"[{self.client_id}] Result: extroversion={extroversion}, style={style}, instructions={instructions}")
                
                print(f"[{self.client_id}] Checking for additional messages...")
                additional_messages = self.receive_all_pending_messages()
                
                for additional_msg in additional_messages:
                    additional_parsed = self.parse_json_message(additional_msg)
                    if additional_parsed and additional_parsed.get('type') == 'state_change':
                        print(f"[{self.client_id}] Found additional state_change!")
                        new_state = additional_parsed.get('new_state', '')
                        personality_state = additional_parsed.get('personality', '')
                        
                        print(f"[{self.client_id}] State change: {new_state}, personality: {personality_state}")
                        
                        if new_state == 'follow_up':
                            print(f"[{self.client_id}] Sending READY_FOR_FOLLOWUP confirmation...")
                            if self.send_message("READY_FOR_FOLLOWUP"):
                                print(f"[{self.client_id}] Confirmation sent successfully!")
                                self.stress_test_followup()
                                return True
                            else:
                                print(f"[{self.client_id}] Error sending confirmation!")
                                return False
                
                print(f"[{self.client_id}] Waiting for state_change...")
                
            elif msg_type == 'state_change':
                new_state = parsed.get('new_state', '')
                personality_state = parsed.get('personality', '')
                
                print(f"[{self.client_id}] State change: {new_state}, personality: {personality_state}")
                
                if new_state == 'follow_up':
                    print(f"[{self.client_id}] Sending READY_FOR_FOLLOWUP confirmation...")
                    if self.send_message("READY_FOR_FOLLOWUP"):
                        print(f"[{self.client_id}] Confirmation sent successfully!")
                        self.stress_test_followup()
                        return True
                    else:
                        print(f"[{self.client_id}] Error sending confirmation!")
                        return False
                    
            elif msg_type in ['gpt_closing', 'closing']:
                final_message = parsed.get('message', '')
                print(f"[{self.client_id}] Final message: {final_message}")
                break
                
        print(f"[{self.client_id}] Assessment completato!")
        return len(self.errors) == 0
    
    def stress_test_followup(self, num_followup_questions=3):
        """Gestisce le domande di follow-up"""
        print(f"[{self.client_id}] Handling follow-up...")
        
        followup_responses = [
            "Mi piace stare con le persone, ma a volte ho bisogno di tempo per me",
            "Dipende dalla situazione e dal mio umore",
            "Preferisco piccoli gruppi di amici intimi",
            "Le situazioni nuove mi rendono nervoso all'inizio",
            "Mi sento più energico dopo aver parlato con persone positive"
        ]
        
        for i in range(num_followup_questions):
            message = self.receive_message()
            if not message:
                print(f"[{self.client_id}] No follow-up question received")
                break
                
            parsed = self.parse_json_message(message)
            if parsed and parsed.get('type') == 'gpt_ask':
                question = parsed.get('question', '')
                print(f"[{self.client_id}] Follow-up Q{i+1}: {question[:100]}...")
                
                response = random.choice(followup_responses)
                if not self.send_message(response):
                    break
                    
                print(f"[{self.client_id}] Answered: {response}")
                time.sleep(random.uniform(0.5, 1.5))
            elif parsed and parsed.get('type') in ['gpt_closing', 'closing']:
                final_message = parsed.get('message', '')
                print(f"[{self.client_id}] Follow-up terminato: {final_message}")
                break
                
        print(f"[{self.client_id}] Follow-up completato!")

def run_single_client(client_id, response_pattern="random", delay_before_start=0):
    """Esegue un singolo client"""
    if delay_before_start > 0:
        time.sleep(delay_before_start)
    
    client = IoRobotClient(client_id=client_id)
    
    start_time = time.time()
    success = False
    
    try:
        if client.connect():
            success = client.complete_personality_assessment(response_pattern)
    except Exception as e:
        print(f"[{client_id}] Error during test: {e}")
    finally:
        client.disconnect()
    
    end_time = time.time()
    duration = end_time - start_time
    
    return {
        'client_id': client_id,
        'success': success,
        'duration': duration,
        'responses_count': len(client.responses),
        'errors_count': len(client.errors),
        'errors': client.errors
    }

def stress_test_concurrent_connections(num_clients=10, max_workers=None):
    """Test con connessioni concorrenti"""
    print(f"\n{'='*60}")
    print(f"STRESS TEST: {num_clients} concurrent connections")
    print(f"{'='*60}")
    
    if max_workers is None:
        max_workers = min(num_clients, 20)
    
    patterns = ["random", "introvert", "extrovert", "neutral"]
    
    results = []
    start_time = time.time()
    
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = []
        for i in range(num_clients):
            client_id = f"stress_client_{i+1:03d}"
            pattern = patterns[i % len(patterns)]
            delay = random.uniform(0, 2.0)
            
            future = executor.submit(run_single_client, client_id, pattern, delay)
            futures.append(future)
        
        for future in as_completed(futures):
            try:
                result = future.result(timeout=120)
                results.append(result)
                
                status = "✓ SUCCESS" if result['success'] else "✗ FAILED"
                print(f"[{result['client_id']}] {status} - {result['duration']:.2f}s - "
                      f"Responses: {result['responses_count']} - Errors: {result['errors_count']}")
                      
            except Exception as e:
                print(f"Client failed with exception: {e}")
                results.append({
                    'client_id': 'unknown',
                    'success': False,
                    'duration': 0,
                    'responses_count': 0,
                    'errors_count': 1,
                    'errors': [str(e)]
                })
    
    end_time = time.time()
    total_duration = end_time - start_time
    
    print(f"\n{'='*60}")
    print(f"STRESS TEST RESULTS")
    print(f"{'='*60}")
    print(f"Total clients: {num_clients}")
    print(f"Total time: {total_duration:.2f}s")
    print(f"Successes: {sum(1 for r in results if r['success'])}")
    print(f"Failures: {sum(1 for r in results if not r['success'])}")
    print(f"Average duration per client: {sum(r['duration'] for r in results) / len(results):.2f}s")
    print(f"Total responses sent: {sum(r['responses_count'] for r in results)}")
    print(f"Total errors: {sum(r['errors_count'] for r in results)}")
    
    all_errors = []
    for result in results:
        all_errors.extend(result['errors'])
    
    if all_errors:
        print(f"\nERRORS DETECTED ({len(all_errors)}):")
        for error in set(all_errors):
            count = all_errors.count(error)
            print(f"  - {error} (x{count})")
    
    return results

def test_single_connection():
    """Test con una singola connessione per debug"""
    print(f"\n{'='*60}")
    print(f"SINGLE CONNECTION TEST (DEBUG)")
    print(f"{'='*60}")
    
    result = run_single_client("debug_client", "random")
    
    print(f"\nResult:")
    print(f"  Success: {result['success']}")
    print(f"  Duration: {result['duration']:.2f}s")
    print(f"  Responses: {result['responses_count']}")
    print(f"  Errors: {result['errors_count']}")
    
    if result['errors']:
        print(f"  Error details: {result['errors']}")
    
    return result

def test_rapid_connections(num_clients=5, interval=0.1):
    """Test con connessioni rapide per testare la gestione delle risorse"""
    print(f"\n{'='*60}")
    print(f"RAPID CONNECTIONS TEST: {num_clients} clients, interval {interval}s")
    print(f"{'='*60}")
    
    results = []
    
    for i in range(num_clients):
        client_id = f"rapid_client_{i+1}"
        print(f"Starting {client_id}...")
        
        result = run_single_client(client_id, "random")
        results.append(result)
        
        if i < num_clients - 1:
            time.sleep(interval)
    
    successes = sum(1 for r in results if r['success'])
    avg_duration = sum(r['duration'] for r in results) / len(results)
    
    print(f"\nRapid connections results:")
    print(f"  Successes: {successes}/{num_clients}")
    print(f"  Average duration: {avg_duration:.2f}s")
    
    return results

if __name__ == "__main__":
    print("IoRobot Server Stress Test")
    print("=" * 60)
    
    if len(sys.argv) > 1:
        test_type = sys.argv[1].lower()
        
        if test_type == "single":
            test_single_connection()
        elif test_type == "rapid":
            num = int(sys.argv[2]) if len(sys.argv) > 2 else 5
            test_rapid_connections(num)
        elif test_type == "stress":
            num = int(sys.argv[2]) if len(sys.argv) > 2 else 10
            stress_test_concurrent_connections(num)
        else:
            print(f"Unrecognized test type: {test_type}")
            print("Usage: python test.py [single|rapid|stress] [num_clients]")
    else:
        print("Running all tests...")
        
        test_single_connection()
        time.sleep(2)
        
        test_rapid_connections(3)
        time.sleep(2)
        
        stress_test_concurrent_connections(8)
    
    print("\nTests completed!")