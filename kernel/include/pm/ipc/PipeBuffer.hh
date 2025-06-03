#define PIPE_BUF_SIZE 4096
class PipeBuffer {
private:
    unsigned char buf[PIPE_BUF_SIZE];
    int head = 0;
    int tail = 0;
    int _size = 0;

public:
    bool full() const { return _size == PIPE_BUF_SIZE; }
    bool empty() const { return _size == 0; }

    bool push(unsigned char byte) {
        if (full()) return false;
        buf[tail] = byte;
        tail = (tail + 1) % PIPE_BUF_SIZE;
        _size++;
        return true;
    }

    bool pop() {
        if (empty()) return false;
        head = (head + 1) % PIPE_BUF_SIZE;
        _size--;
        return true;
    }

    unsigned char peek() const {
        if (empty()) return 0; // or handle error
        return buf[head];
    }

    int size() const { return _size; }
};
