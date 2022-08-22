from functools import reduce
import operator as op
import math

#Extracts the message bits and the corrects the message if there is a bit fli[p]
def decode(block):
    binary = bin(int(block, 16))[2:]
    binary = "0"*(40-len(binary))+binary
    binary = [int(i) for i in binary]
    pos = reduce(op.xor,[i for i,bit in enumerate(binary) if bit])
    binary[pos] = 0 if binary[pos] else 1
    msg_binary = ""
    for i in range(1,39):
        if math.ceil(math.log2(i)) != math.floor(math.log2(i)):
            msg_binary=msg_binary+str(binary[i])
    if binary[0] == 1:
        pos = -1
    return msg_binary,pos
    

#Code finds the Coded message from the binary values
def get_msg(msg_binary):
    binary_int = int("".join(msg_binary), 2)

    # Getting the byte number
    byte_number = binary_int.bit_length() + 7 // 8
    
    # Getting an array of bytes
    binary_array = binary_int.to_bytes(byte_number, "big")
    
    # Converting the array into ASCII text
    ascii_text = binary_array.decode()
    
    # Getting the ASCII value
    return(ascii_text)

def main(code):
    MSG = ""
    index_flip=[]
    num_block=0
    for i in range(0,math.ceil(len(code)/10)):
        num_block+=1
        block = code[i*10:(i+1)*10]
        #Acquire the msg bits and the position of error
        msg,pos=decode(block)   
        #If pos is -1, then no error in the Block
        if pos!=-1:
            index_flip.append(str(pos))
        MSG+=msg
    Decoded = get_msg(MSG)

    #Display Block
    print("Text: ",Decoded)
    print("code word: ", code)
    if len(index_flip)==0:
        print("Bit flip idx: Not flipped")
    else:
        print("Bit flip idx: "," ".join(index_flip))
    print("Num Blocks: ",num_block)

#The Coded message
code = '044B5281EE2E8BCC8942220109C9D2463BA1D0D0061BBDB1486A839085726203A5B8E044B31D89E44F2B05C9760A6101855E2F2181D1504EA981ADD80EFF0DAD660A03D995E44E2901DDE82F1325AFD206D39C81E83EC3A5C9E8662B97B85C'
main(code)