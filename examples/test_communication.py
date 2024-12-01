import zmq_interface as zi
import pickle
import time
import numpy as np


def test_communication():
    server = zi.ZMQServer("ipc:///tmp/feeds/0")
    client = zi.ZMQClient("ipc:///tmp/feeds/0")

    server.add_topic("test", 10000)

    rand_data = np.random.rand(1000000)
    print(f"Data size: {rand_data.nbytes / 1024**2:.3f}MB")

    for k in range(10):
        start_time = time.time()
        pickle_data = pickle.dumps(rand_data)
        dump_end_time = time.time()
        server.put_data("test", pickle_data)
        send_end_time = time.time()
        time.sleep(0.1)
        retrieve_start_time = time.time()
        retrieved_data = client.request_latest("test")
        retrieve_end_time = time.time()
        received_data = pickle.loads(retrieved_data)
        print(
            f"Time elapsed: dump: {dump_end_time - start_time:.4f}, send: {send_end_time - dump_end_time: .4f}, retrieve: {retrieve_end_time - retrieve_start_time:.4f}, load: {time.time() - retrieve_end_time:.4f}"
        )
        print(np.allclose(received_data, rand_data))
        # assert np.allclose(retrieved_data, received_data)
        # time.sleep(0.1)

    print("Start requesting")
    print([pickle.loads(data) for data in client.request_all("test")])
    print("End requesting")


if __name__ == "__main__":
    test_communication()
