from zmq_interface import ZMQClient
import time
import numpy as np
import numpy.typing as npt


def test_client():
    client = ZMQClient("test_zmq_client", "tcp://localhost:5555")
    print("Client created")

    while True:
        start_time = time.time()
        raw_data_list, timestamps = client.peek_data("test", "latest", 1)
        end_peeking_time = time.time()

        if raw_data_list:
            data = np.frombuffer(raw_data_list[0], dtype=np.float64)

            print(
                f"Received data: shape: {data.shape}, size: {data.nbytes / 1024**2:.3f}MB, peeking time: {end_peeking_time - start_time:.3f}s"
            )
        time.sleep(0.1)


if __name__ == "__main__":
    test_client()
