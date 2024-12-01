import zmq_interface as zi
import pickle
import time


def test_communication():
    server = zi.ZMQServer("tcp://localhost:12345")
    client = zi.ZMQClient("tcp://localhost:12345")

    server.add_topic("test", 10000)
    print(server.get_all_topic_names())

    for k in range(10):
        pickle_data = pickle.dumps(k)
        id1 = id(pickle_data)
        server.put_data("test", pickle_data)
        del pickle_data
        retrieved_data = server.get_latest_data("test")
        time.sleep(0.1)
        print(pickle.loads(client.request_latest("test")))
        # time.sleep(0.1)

    print("Start requesting")
    print([pickle.loads(data) for data in client.request_all("test")])
    print("End requesting")


if __name__ == "__main__":
    test_communication()
