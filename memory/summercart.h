#ifndef SUMMERCART_H
#define SUMMERCART_H

struct summercart
{
	char buffer[8192];
	unsigned long status;
	unsigned long data0;
	unsigned long data1;
	unsigned long sd_sector;
	char cfg_rom_write;
	char sd_byteswap;
	char unlock;
	char lock_seq;
	char pad[492];
};

extern struct summercart summercart;

void save_summercart(const char *filename);
void load_summercart(const char *filename);
void init_summercart();
unsigned long read_summercart(unsigned long address);
void write_summercart(unsigned long address, unsigned long value);

#endif
