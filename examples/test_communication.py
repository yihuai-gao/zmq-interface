import zmq_interface as zi
import pickle


def test_communication():
    server = zi.ZMQServer("tcp://localhost:12345")
    client = zi.ZMQClient("tcp://localhost:12345")

    server.add_topic("test", 10000)
    print(server.get_all_topic_names())

    for k in range(10):
        server.put_data("test", pickle.dumps(k))
        print(type(pickle.dumps(k)))
        print(type(server.get_latest_data("test")))
        # print(server.get_latest_data("test").hex())
    # print(client.request_latest("test").hex())
    # print(pickle.loads(client.request_latest("test")))

    # print([pickle.loads(data) for data in cilent.request_all("test")])


if __name__ == "__main__":
    test_communication()
