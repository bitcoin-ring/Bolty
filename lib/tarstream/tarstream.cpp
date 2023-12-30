#include <Arduino.h>
#include "tarstream.h"

TarStream::TarStream(uint8_t *buffer, unsigned int size){
  this->databuffer = buffer;
  this->databuffer_len = size;
}

size_t TarStream::write(const uint8_t *buffer, size_t size)
{
  return 0;
}

size_t TarStream::write(uint8_t data){
  return 0;
}

int TarStream::available() {
    return length();
}

int TarStream::read() {
    if(available()) {
        uint8_t c = this->databuffer[cur];
        this->cur++;
        return c;

    }
    return -1;
}

int TarStream::peek() {
    if(available()) {
        char c = this->databuffer[this->cur];
        return c;
    }
    return -1;
}

int TarStream::length() {
  if (cur >= this->databuffer_len){
    return 0;
  }
  return this->databuffer_len-this->cur;
}

void TarStream::flush() {
}
