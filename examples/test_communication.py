import zmq_interface as zi
import pickle
import time


def test_communication():
    server = zi.ZMQServer("tcp://localhost:12345")
    # client = zi.ZMQClient("tcp://localhost:12345")

    server.add_topic("test", 10000)
    # print(server.get_all_topic_names())

    for k in range(10):
        pickle_data = pickle.dumps(k)
        server.put_data("test", pickle_data)
        retrieved_data = server.get_latest_data("test")
        copied_data = bytes(pickle_data)
        print(id(pickle_data), id(retrieved_data), id(copied_data))
        time.sleep(1)
        # print(server.get_latest_data_ptr("test").hex())
    # print(client.request_latest("test").hex())
    # print(pickle.loads(client.request_latest("test")))

    # print([pickle.loads(data) for data in cilent.request_all("test")])


if __name__ == "__main__":
    test_communication()
