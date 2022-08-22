#!/usr/bin/env python3
import matplotlib.pyplot as plt
import argparse
from math import ceil
import random
import numpy as np



#Get Arguments from Command Line
def get_args() :

    parser = argparse.ArgumentParser()
    parser.add_argument("-i", type=float, nargs='?', default = 1.0, help="K_i : The initial congestion window (CW). Default value is 1.")
    parser.add_argument("-m", type=float, nargs='?', default = 1.0, help="K_m : The multiplier of the Congestion Window, during the exponential growth phase. Default value is 1.")
    parser.add_argument("-n", type=float, nargs='?', default = 1.0, help="K_n : The multiplier of the Congestion Window, during the linear growth phase. Default value is 1.")
    parser.add_argument("-f", type=float, nargs='?', default = 0.3, help="K_f : The multiplier when a timeout occurs.")
    parser.add_argument("-s", type=float, nargs='?', default = 0.99, help="P_s : The probability of receiving the ACK packet for a given segment before its timeout occurs.")
    parser.add_argument("-T", type=int, nargs='?', default = 1000, help="T : The total number of segments to be sent before the emulation stops.")
    parser.add_argument("-o", type=str, nargs='?', default = "output.txt", help="The output file.")
    
    args = parser.parse_args()
    return args

#Checking if the ack for a segment is received.
def Ack_for_segments(P_s, packets_to_be_sent) :

    for num in range(packets_to_be_sent) :
        if(random.uniform(0,1) < (1-P_s)) :   #if ack not returned to sender
            return num      
    return packets_to_be_sent

def congestion_control_algorithm(K_i=1, K_m=1, K_n=1, K_f=0.3
, P_s=0.99, T=1000, flag=0, file="output", image_file = "Image"):
    
    #Random values will be the same for a given value of seed, to help make observations. 
    np.random.seed(100)
    random.seed(10)
    
    #Maximum Segment Size
    MSS = 1 # in KB

    #Receiver Window Size
    RWS = 1000 # in KB

    #Congestion Threshold
    Congestion_threshold = RWS/2

    #Congestion Window
    CW = K_i * MSS

    #Record of the Congestion Window Size
    CW_list = []

    #Slow start State
    slow_start_state = True

    #Number of Packets sent
    num_packets_sent = 0

    #Opening the output file for writing
    if flag==0:
        f = open(str(file), "w")
    Update_No=0

    #Iterate until num of segments sent is less than the num of segments to be sent
    while(num_packets_sent < T):

        #Recording the congestion window size
        if flag==0:
            f.write("Update No : {}, CW = {}\n".format(Update_No, CW))
        CW_list.append(CW)

        #number of packets to be sent in the particular slot.
        num_packets_sending = ceil(CW/MSS)  

        #Number of packets for which acknowledgment is received
        num_packets_ack = Ack_for_segments(P_s,num_packets_sending)

        #Total number of packets sent so far
        num_packets_sent += num_packets_ack 

        #Check to see if a segment was dropped
        if num_packets_ack == num_packets_sending:
            #If window is in exponential state
            if slow_start_state == True : 
                CW = min(CW + K_m*MSS, RWS)
                if CW >= Congestion_threshold:
                    CW = Congestion_threshold
                    slow_start_state = False
            #If window is in Linear state
            else:
                CW = min(CW + K_n * (MSS*MSS) / CW  , RWS)

        #Packet Dropped
        else:
            #Resetting Threshold and Congestion Window
            Congestion_threshold = CW/2
            # print(Congestion_threshold)
            CW = max(1, K_f * CW)
            # print(CW)
            #Slow start state check
            if CW < Congestion_threshold:
                slow_start_state = True
        Update_No+=1

    if flag==0:
        f.close()
    # print(CW_list)

    plt.figure()
    plt.plot(range(len(CW_list)), CW_list)
    plt.ylabel("Congestion window length (in KB)")
    plt.xlabel("Update Number")
    plt.grid()
    if flag==0:
        plt.savefig(image_file)
        plt.show()
    else:
        file_name = f"plots/ki={str(K_i)}, km={str(K_m)}, kn={str(K_n)}, kf={str(K_f)}, ps={str(P_s)}"
        plt.savefig(f"{file_name}.png")
    
    


if __name__ == "__main__":
    #Get parameters
    args = get_args()
    
    #Algorithm
    congestion_control_algorithm(args.i, args.m, args.n, args.f, args.s, args.T,0,args.o)

    