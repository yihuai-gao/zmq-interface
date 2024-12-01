import zmq_interface as zi
import pickle
import time
import numpy as np


def test_communication():
    server = zi.ZMQServer("ipc:///tmp/feeds/0")
    client = zi.ZMQClient("ipc:///tmp/feeds/0")
    print("Server and client created")


    time.sleep(0.1)
    del client
    print("Client deleted")
    time.sleep(0.1)
    del server
    print("Server deleted")


    # server.add_topic("test", 10)
    # rand_data_list = []
    # for k in range(10):
    #     rand_data = np.random.rand(1000000)
    #     rand_data_list.append(rand_data)
    #     start_time = time.time()
    #     pickle_data = pickle.dumps(rand_data)
    #     dump_end_time = time.time()
    #     server.put_data("test", pickle_data)
    #     send_end_time = time.time()
    #     time.sleep(0.01)
    #     retrieve_start_time = time.time()
    #     retrieved_data = client.request_latest("test")
    #     retrieve_end_time = time.time()
    #     received_data = pickle.loads(retrieved_data)
    #     print(
    #         f"Data size: {rand_data.nbytes / 1024**2:.3f}MB. dump: {dump_end_time - start_time:.4f}s, send: {send_end_time - dump_end_time: .4f}s, retrieve: {retrieve_end_time - retrieve_start_time:.4f}s, load: {time.time() - retrieve_end_time:.4f}s, correctness: {np.allclose(received_data, rand_data)}"
    #     )

    # start_time = time.time()
    # all_pickle_data = client.request_all("test")
    # request_end_time = time.time()
    # all_data = [pickle.loads(data) for data in all_pickle_data]
    # loads_end_time = time.time()
    # correctness = [np.allclose(a, b) for a, b in zip(all_data, rand_data_list)]
    # print(
    #     f"Request all data. Time: {request_end_time - start_time:.4f}s, load time: {loads_end_time - request_end_time: .4f}, correctness: {np.all(correctness)}"
    # )

    # last_k_pickle_data = client.request_last_k("test", 5)
    # last_k_data = [pickle.loads(data) for data in last_k_pickle_data]
    # correctness = [np.allclose(a, b) for a, b in zip(last_k_data, rand_data_list[-5:])]
    # print(f"Request last 5 data. Correctness: {np.all(correctness)}")



if __name__ == "__main__":
    test_communication()
