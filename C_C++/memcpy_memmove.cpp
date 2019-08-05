void* _memcpy(void* dest, const void* src, size_t count{
	assert(src != nullptr && dest != nullptr);
	char* tmp_dest = (char*)dest;
	const char* tmp_src = (const char*)src;
	while(count--)
		*tmp_dest++ = *tmp_src++;
	return dest;
}

void* _memmove(void* dest, const void* src, size_t count){
	assert(src != nullptr && dest != nullptr);
	char* tmp_dest = (char*)dest;
	const char* tmp_src = (const char*)src;
	if(dest <= src){
		while(count--)
			*tmp_dest++ = *tmp_src++;
	}
	else{
		dest += count - 1;
		src  += count - 1;
		while(count--)
			*tmp_dest-- = *tmp_src--;
	}
	return dest;
}
