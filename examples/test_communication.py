import zmq_interface as zi
import pickle


def test_communication():
    server = zi.ZMQServer("tcp://localhost:12345")
    client = zi.ZMQClient("tcp://localhost:12345")

    server.add_topic("test", 10000)

    for k in range(1000):
        server.put_data("test", pickle.dumps(k))

    print(pickle.loads(client.request_latest("test")))

    print([pickle.loads(data) for data in cilent.request_all("test")])


if __name__ == "__main__":
    test_communication()
