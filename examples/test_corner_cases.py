import zmq_interface as zi
import time
import pickle
import numpy as np


def test_corner_cases():
    server = zi.ZMQServer("test_zmq_server", "ipc:///tmp/feeds/0")
    client = zi.ZMQClient("test_zmq_client", "ipc:///tmp/feeds/0")

    print("Server and client created")
    server.add_topic("test", 10)
    # server.put_data("test", pickle.dumps(np.random.rand(10)))

    data, timestamp = client.request_latest("test")  # No data available
    print(f"request_latest if no data available: {data}, {timestamp}")

    data, timestamp = client.request_all("test")  # No data available
    print(f"request_all if no data available: {data}, {timestamp}")

    data, timestamp = client.request_last_k("test", 5)  # No data available
    print(f"request_last_k if no data available: {data}, {timestamp}")


if __name__ == "__main__":
    test_corner_cases()
