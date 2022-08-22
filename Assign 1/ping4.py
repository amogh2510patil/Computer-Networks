#Code to ping a server 4 times
import os
def myping(host):
    response = os.system("ping " + host)
    
    if response == 0:
        return True
    else:
        return False
        
myping("139.130.4.5")
myping("139.130.4.5")
myping("139.130.4.5")
myping("139.130.4.5")
