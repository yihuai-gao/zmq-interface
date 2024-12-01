import zmq_interface as zi
import pickle
import time
import numpy as np


def test_communication():
    server = zi.ZMQServer("ipc:///tmp/feeds/0")
    client = zi.ZMQClient("ipc:///tmp/feeds/0")

    server.add_topic("test", 10)

    for k in range(10):
        rand_data = np.random.rand(1000000)
        start_time = time.time()
        pickle_data = pickle.dumps(rand_data)
        dump_end_time = time.time()
        server.put_data("test", pickle_data)
        send_end_time = time.time()
        time.sleep(0.01)
        retrieve_start_time = time.time()
        retrieved_data = client.request_latest("test")
        retrieve_end_time = time.time()
        received_data = pickle.loads(retrieved_data)
        print(
            f"Data size: {rand_data.nbytes / 1024**2:.3f}MB. dump: {dump_end_time - start_time:.4f}s, send: {send_end_time - dump_end_time: .4f}s, retrieve: {retrieve_end_time - retrieve_start_time:.4f}s, load: {time.time() - retrieve_end_time:.4f}s, correctness: {np.allclose(received_data, rand_data)}"
        )

    print("Start requesting")
    print([pickle.loads(data) for data in client.request_all("test")])
    print("End requesting")


if __name__ == "__main__":
    test_communication()
