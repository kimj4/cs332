# Test 1: Continuously add to a list to use up memory
# Author: Sherri Goings
# Last Modified: 5/21/2016

import sys

def test1(L):
    """add a string to the list L continuously"""
    
    while (True):
        if len(L)%10000000==0:
            print("cur len: ", len(L))
        try: 
            L.append("this is a string that uses some memory...")
        except:
            print("too big")
            input("hit enter to quit")
            

def main():
	# run test 1
    L = []
    test1(L)
   
    
if __name__=="__main__":
    main()



