import platform
import subprocess
import numpy as np
from math import radians, cos, sin, asin, sqrt
import re
import requests
import json
import matplotlib.pyplot as plt

def haversine(lon1, lat1, lon2, lat2):
    """
    Calculate the great circle distance in kilometers between two points
    on the earth (specified in decimal degrees)
    """
    # convert decimal degrees to radians
    lon1, lat1, lon2, lat2 = map(radians, [lon1, lat1, lon2, lat2])
    # haversine formula
    dlon = lon2 - lon1
    dlat = lat2 - lat1
    a = sin(dlat/2)**2 + cos(lat1) * cos(lat2) * sin(dlon/2)**2
    c = 2 * asin(sqrt(a))
    r = 6371 # Radius of earth in kilometers.
    return c * r

#Find the Longitute, Latitude and City of a place using API's
def long_lat(row):
    """This function calls the api and return the response"""
    url = f"https://freegeoip.app/json/{row}"       # getting records from getting ip address
    headers = {
        'accept': "application/json",
        'content-type': "application/json"
        }
    response = requests.request("GET", url, headers=headers)
    respond = json.loads(response.text)
    lat,long=respond['latitude'],respond['longitude']
    if respond['city']!='':
        loc = respond['city']
    elif respond['region_name']!='':
        loc = respond['region_name']
    else:
        loc = respond['country_name']
    return lat,long,loc

#Calculate the distance between two locations
def calc_distances(ip_addr):
    dist = []
    locations = []
    for ip in ip_addr:
        lat, long, loc = long_lat(ip)
        dist.append(haversine(float(80.23),float(12.99),float(long),float(lat)))
        locations.append(loc)
    return dist,locations 

#Finding Round Trip Times of 10 pings for a given Ip
def myping(host):
    parameter = '-n' if platform.system().lower()=='windows' else '-c'
    i = 1
    time = []
    rtt=[]
    regex = 'time='+'\d+'  
    while i<11:
        command = ['ping', parameter, '1', host]
        try:
            response = subprocess.run(command,text=True,timeout=10,capture_output=True,check=True)
            times = re.findall(regex, response.stdout)
            rtt.append(times[0].split('=')[1])
            i=i+1
        except:
            pass    
    return(rtt)

#Reading the ping file
def read_pingtext():
    with open("ping-servers.txt", "r") as f:
        lines = [line.rstrip() for line in f]
    return(lines)

file_out = open('rtt-log.log',"w")
file_out.close()
ip_addr = read_pingtext()       #obtain Ip adresses
distances,locations = calc_distances(ip_addr)       #Get Distances and City Names
rtt = []
dis = []
#Getting round trip times and logging
with open('rtt-log.log',"a") as file_out:
    for i in range(len(ip_addr)):
        rtt_vals= myping(ip_addr[i])
        rtt.append([float(val) for val in rtt_vals])
        dis.append([distances[i]]*10)
        rtt_string = ",".join(rtt_vals)
        file_out.write("Chennai,\t"+ip_addr[i]+", \t"+str(locations[i])+", \t"+rtt_string+"\n")
        print("Chennai,\t"+ip_addr[i]+", \t"+str(locations[i])+", \t"+rtt_string+", \t"+str(distances[i])+"km \n")

#speed of light calculation
light_time = np.array([float(2*dist/300) for dist in distances])
rtt = np.array(rtt)
dis = np.array(dis)
mean_rtt = rtt.mean(axis=1)

#Plot of rtt vs Distance
plt.scatter(dis, rtt, label = 'rtt')
plt.scatter(distances, mean_rtt, color='orange', label = 'Mean rtt')
plt.scatter(distances, light_time, marker='X', color='red', label = 'Light rtt')
plt.legend()
plt.grid()
plt.xlabel("Distances")
plt.ylabel("Round Trip time")
plt.savefig("RTT_vs_Distance.png")
plt.show()


#Slowdown wrt to speed of light
slowdown = mean_rtt/light_time
Mean_slowdown = np.mean(slowdown)
print("Mean Slowdown: ",Mean_slowdown)


