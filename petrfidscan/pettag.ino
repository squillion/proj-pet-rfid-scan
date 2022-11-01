// 10 Country code according to ISO 3166,
// 38 ID bits
#define PETTAG_DATA_SIZE 7
#define PETTAG_STR_SIZE 16
#define PETTAG_RAW_STR_SIZE 15

#define PETTAG_INPUT_SIZE 13
#define CIRCULAR_BUF_SIZE PETTAG_INPUT_SIZE

typedef void (*petid_callback)(char *, char *);
petid_callback callback = NULL;

struct circular_buffer {
  unsigned char buffer[CIRCULAR_BUF_SIZE];
  size_t head;
  size_t tail;
  size_t len;
};

struct circular_buffer inputbuffer = {{0}};

void circular_reset(unsigned char byte) {
  inputbuffer.head = 0;
  inputbuffer.tail = 0;
  inputbuffer.len = 0;
}

void circular_put(char byte) {
  inputbuffer.buffer[inputbuffer.head] = byte;
  inputbuffer.head = (++inputbuffer.head) % CIRCULAR_BUF_SIZE;
  if ((++inputbuffer.len) > CIRCULAR_BUF_SIZE) {
    inputbuffer.len = CIRCULAR_BUF_SIZE;
    inputbuffer.tail = (++inputbuffer.tail) % CIRCULAR_BUF_SIZE;
  }
}

size_t circular_len() {
  return inputbuffer.len;
}

unsigned char circular_get(size_t pos) {
  size_t idx = (inputbuffer.tail + pos) % CIRCULAR_BUF_SIZE;
  return inputbuffer.buffer[idx];
}

bool check_pet_tag_read(unsigned char* data) {
  if (circular_len() == PETTAG_INPUT_SIZE &&
      circular_get(0) == 0xAA &&
      circular_get(12) == 0xBB) {

    unsigned char checksum = 0;
    for (int i = 1; i < (PETTAG_INPUT_SIZE - 2); i++) {
      checksum = checksum ^ circular_get(i);
    }
    if (checksum == circular_get(11)) {
      for (int i = 4; i < (PETTAG_INPUT_SIZE - 2); i++) {
        data[i - 4] = circular_get(i);
      }
      return true;
    }
  }
  return false;
}

void byte_to_hex_str(char* str, unsigned char byte) {
  str[0] = (byte >> 4) + 0x30;
  if (str[0] > 0x39) str[0] += 7;
  str[1] = (byte & 0x0f) + 0x30;
  if (str[1] > 0x39) str[1] += 7;
}

void bits_to_bcd(char* str, int size, int bit) {
  for (int c = 0; c < size; c++) {
    if (str[c] >= 5) {
      str[c] = str[c] + 3;
    }
  }

  for  (int c = 0; c < (size - 1); c++) {
    str[c] = (str[c] << 1) & 0xF;
    str[c] =  str[c] | ((str[c + 1] >> 3) & 1);
  }

  str[size - 1] = (str[size - 1] << 1) & 0xF;
  str[size - 1] =  str[size - 1] | (bit & 1);
}

char petid_raw[PETTAG_RAW_STR_SIZE];
char petid[PETTAG_STR_SIZE];

void process_pettag_data(unsigned char* data) {
  for (int i = 0; i < PETTAG_DATA_SIZE; i++) {
    byte_to_hex_str(&petid_raw[i * 2], data[i]);
  }
  petid_raw[PETTAG_RAW_STR_SIZE - 1] = '\0';

  memset(petid, 0, sizeof(petid));

  // Country code
  for (int i = 0; i < 16; i++) {
    int idx = (i / 8);
    int shift = 7 - (i % 8);
    int bit = (data[idx] >> shift) & 1;
    bits_to_bcd(petid, 3, bit);
  }

  // ID
  for (int i = 16; i < 56; i++) {
    int idx = (i / 8);
    int shift = 7 - (i % 8);
    int bit = (data[idx] >> shift) & 1;
    bits_to_bcd(petid + 3, 12, bit);
  }

  // Convert to string
  for (int c = 0; c < PETTAG_STR_SIZE; c++) {
    petid[c] = petid[c] + '0';
  }
  petid[PETTAG_STR_SIZE - 1] = '\0';
}

void pettag_data_put(unsigned char byte) {
  unsigned char data[PETTAG_DATA_SIZE];
  circular_put(byte);
  if (check_pet_tag_read(data)) {
    process_pettag_data(data);

    if (callback != NULL) {
      callback(petid, petid_raw);
    }
  }
}

void petid_set_callback(void (*petid_cb)(char *, char *)) {
  callback = petid_cb;
}
