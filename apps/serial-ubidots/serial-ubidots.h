/*----------------------------------------------*/
#define UBIDOTS_MSG_APILEN 40
#define UBIDOTS_MSG_KEYLEN 24
#define UBIDOTS_MSG_16B_NUM 1
#define UBIDOTS_MSG_LEN (UBIDOTS_MSG_KEYLEN + UBIDOTS_MSG_APILEN + (UBIDOTS_MSG_16B_NUM*2) + 2)
/*----------------------------------------------*/
struct ubidots_msg_t {
	uint8_t id;
	uint8_t len;
	uint16_t value[UBIDOTS_MSG_16B_NUM];
	
	#ifdef UBIDOTS_MSG_APILEN
		char api_key[UBIDOTS_MSG_APILEN];
	#endif
	
	#ifdef UBIDOTS_MSG_KEYLEN
		char var_key[UBIDOTS_MSG_KEYLEN];
	#endif
};
/*----------------------------------------------*/
void send_to_ubidots(const char *api, const char *id, uint16_t *val);
/*----------------------------------------------*/