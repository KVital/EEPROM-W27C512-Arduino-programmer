# -*- coding: utf-8 -*-
"""
Created on Thu Feb  5 15:57:46 2026

@author: vkhodnevych
"""
import random
rom=bytearray([0xea]*32768*2)

rom[0]=0xa9
rom[1]=0x42

rom[2]=0x8d

rom[0xfffc]=0x00
rom[0xfffd]=0x80

# fill the rest with random values
#for i in range(len(rom)):
#    rom[i] = random.randint(0, 0xFF)


with open("rom.bin","wb") as out_file:
    out_file.write(rom);