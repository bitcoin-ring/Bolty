#ifndef TARSTREAM_H
#define TARSTREAM_H
class TarStream: public Stream, public String
{
private:
    long cur = 0;
    int length();
    uint8_t *databuffer;
    unsigned int databuffer_len;

public:
    TarStream(uint8_t *buffer, unsigned int size);
    size_t write(const uint8_t *buffer, size_t size) override;
    size_t write(uint8_t data) override;
    int available() override;
    int read() override;
    int peek() override;
    void flush() override;
};

#endif
