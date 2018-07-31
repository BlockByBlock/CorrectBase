import pickle
import socket
 
class foo(object):   pass
 
#num of satelite, satelite type, satelite strength, satelite mode (not in list)
dList = [[22],['USA1','China1','USA2','China2','USA3','China3'],[20,25,30,35,40,45]]
objList = pickle.dumps ( dList )
 
client = socket.socket ( socket.AF_INET, socket.SOCK_STREAM )
client.connect (('localhost', 5005))
client.send(objList)
client.close()
