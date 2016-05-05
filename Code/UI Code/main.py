"""
Team Id - 5
Author List - Shubham Jain, Palash Kala, Amol Agrawal, Vikas Garg
Filename - main .py
Theme -
Classes - Map(), Bot(), xbeeCommunicator()
Global Variables - 
    commands - Defines all signals that can be sent to bot. Divided into two arrays to differentiate between bots.
    tableX - x coordinates of all tables
    tableY - y coordinates of all tables
    nextLocationX0 - next 'X' location of first bot
    nextLocationY0 - next 'Y' location of first bot
    nextLocationX1 - next 'X' location of second bot
    nextLocationY1 - next 'Y' location of second bot
"""


import XBee
from time import sleep
import simplejson as json
import requests

"""
XBee communication module used for interaction with the bot via self.xbee
"""
commands = [ ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'], ['I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'] ];
tableX = [0, 1, 2, 2, 1]
tableY = [0, 2, 2, 3, 3]
nextLocationX0=0
nextLocationY0=0
nextLocationX1=0
nextLocationY1=0

"""
Class Name - Map()
This stores the complete map of the restaurant. Like what is the terminal point and what is a valid point in map.
"""
class Map():
    """
    Function Name : __init__
    Logic : It is the intialisation of the class Map. The variable change decides the change in position depending on the direction. First array stores change in x direction and second array stores change in y.
    """

    def __init__(self):
        self.change = [[0, 1, 0, -1],[1, 0, -1, 0]]

    """
    Function Name : is_valid_point
    Input : A point -> any point which can be inside or outside of the map.
    Output : Returns whether this point is inside or outside of the map.
    Logic : In our case we just assumed our map a square grid and we just checked the boundaries but in real life map can be of any type and we can check accordingly

    Example : is_valid_point([1,2])
    """
    def is_valid_point(self, point):
        if point[0] < 4 and point[0] > 0 and point[1] < 4 and point[1] > 0:
            return True
        return False

    """
    Function Name : is_terminal_point
    Input : A point -> any point which can be inside or outside of the map.
    Output : Returns whether this point is a terminal point of the map.
    Logic : A terminal point is a point from where bots will take the food and serve to tables. This is the location where someone will put the food on the bot and give the table num in our web app. 
    Example : is_terminal_point([1,2])
    """
    def is_terminal_point(self, point):
        if (point[1] == 0 and point[0] < 4 and point[0] > 0):
            return True
        return False

    """
    Function Name : path
    Input : Current Position and direction of the bot and destination position where bot should reach
    Output : List such that first element of set is array of destination nodes, second is array of directions and third is final direction
    Logic : We start from current position and find the shortest path by BFS on a graph where junctions are the nodes and whitelines between them are the undirected edges.

    Example Call : path(1,0,2,3)
    
    """
    def path(self, currX, currY, currDir, destX, destY):
        
        # queue : Normal BFS queue
        queue = []
        # nodes_inserted : Stores parent point, command taken from parent point to point A, direction at point A for every point A. This is used also to check whether we are processing same node again or not.
        nodes_inserted = {}
        queue.append([currX, currY])
        # point = [parent_point, command_taken_from_parent_to_point, direction_at_point]
        nodes_inserted[(currX, currY)] = [[-1, -1], -1, currDir]

        while len(queue)>0:
            # point : First element of the queue
            point = queue[0]
            # If point is the destination point then just break the loop
            if point[0] == destX and point[1] == destY:
                break
            # direction : The direction of the bot at current point
            direction = nodes_inserted[(point[0],point[1])][2]
            # i : i represents the command being implemented
            # will iterate from 0 to 3 for all 4 directions it can go

            for i in range(4):
                new_direction = (direction+i)%4
                # new_point : New point depends on the point and new_direction variables
                new_point = [point[0] + self.change[0][new_direction],point[1] + self.change[1][new_direction]]
                # If not already executed new point
                if (new_point[0],new_point[1]) not in nodes_inserted:
                    # point should be a valid point and new_point should be a valid point or terminal point
                    if self.is_valid_point(new_point) or (self.is_terminal_point(new_point) and self.is_valid_point(point)):
                        queue.append(new_point)
                        nodes_inserted[(new_point[0],new_point[1])] = [point,i,new_direction]
            # Popping the first element of the queue
            queue.pop(0)

        print nodes_inserted
        curr_point = queue[0]
        path = []
        commands = []
        """
        Starting from the destination and go to parent node again and again till you reach (-1,-1)
        """
        while curr_point[0] != -1 and curr_point[1] != -1:
            path.append(curr_point)
            commands.append(nodes_inserted[(curr_point[0], curr_point[1])][1])
            curr_point = nodes_inserted[(curr_point[0], curr_point[1])][0]
        """
        As we have calculated path from destination we have to reverse it.
        """
        path.reverse()
        commands.reverse()
        path = path[1:]
        commands = commands[1:]
        """
        path is a set of nodes from current position to the destination
        commands is a set of instructions to go forward or left and right at every node in path
        nodes_inserted[(destX, destY)][2]] is final direction of the bot after reaching the destination
        """
        return [path, commands, nodes_inserted[(destX, destY)][2]]

"""
Class Name - Bot()
Every bot will have a instance of the bot class where some primary things of bot will be stored like botId
"""

class Bot():
    """
    Function Name : __init__
    Logic : It is the intialisation of the class Bot. Here we store -
            base point of the bot (X, Y),
            status of the bot (0 : not moving, 1 : moving),
            id (Differnt for every bot),
            next location of the bot (nextX, nextY),
            current direction of the bot (direction)
            
    """
    def __init__(self, xbee, botId, baseX, baseY):
        """
        base = Base point of bot
        X,Y = current location of bot
        direction = current direction of bot
        order_position = position of food on the bot(1 for left and 2 for right) {default 0}
        """
        self.xbee = xbee  # Your serial port name here
        self.id = botId
        self.status = 0
        self.baseX = baseX
        self.X = baseX
        self.nextX = baseX
        self.baseY = baseY
        self.Y = baseY
        self.nextY = baseY
        self.direction = 0
        self.order_position = 0
        self.mapObject = Map()
        self.update_next_pos()

    """
    Function Name : set_location
    Input :  x location, y location and a direction
    Output : Nothing
    Logic : Just set the location and direction variables of class as input

    Example Call : set_location(1,2,2)
    
    """
    
    def set_location(self, locationX, locationY, direction):
        self.X = locationX
        self.Y = locationY
        self.direction = direction

    """
    Function Name : set_destination
    Input :  x location, y location and position of food on the bot
    Output : Nothing
    Logic : First set the order_position of class as input then find the path to reach destination from bot's location by using path function we defined earlier. Path, commands and final direction will come as output from there

    Example Call : set_destination(2,3,1)
    
    """
    def set_destination(self, destinationX, destinationY, order_position):
        print ("BotId ",self.id," assigned ",destinationX, " ",destinationY)
        self.order_position = order_position
        ret = self.mapObject.path(self.X,self.Y, self.direction, destinationX,destinationY)
        self.path = ret[0]
        self.commands = ret[1]
        self.direction = ret[2]
        print "Direction is ",self.direction
        sent = self.xbee.SendStr(commands[self.id-1][6])
        sleep(0.5)

    """
    Function Name : send_status_signal
    Input :  Nothing
    Output : A message
    Logic : It sends signal to bot to send it's status. Then we wait for 0.1 seconds and then we receive some message from bot. 

    Example Call : send_status_signal()
    
    """

    def send_status_signal(self):
        sent = self.xbee.SendStr(commands[self.id-1][7])
        sleep(0.1)
        Msg = self.xbee.Receive()
        print "Bot ID",self.id,"status signal sent"
        return Msg

    """
    Function Name : take_action
    Input :  A message
    Output : Nothing
    Logic : It Sends command according to msg (either move stop or serve)

    Example Call : take_action(Msg) where Msg = self.xbee.Receive()
    
    """

    def take_action(self, Msg):
        """
        botId : this is 0 or 1 for 2 bots while self.id is 1 or 2
        content : Decoded message
        """ 
        botId = self.id-1
        content = Msg[7:-1].decode('ascii')
        print("Msg: " + content)
        if(content == commands[botId][0]):
            print("Only the middle sensor is on")
        elif(content == commands[botId][1]):
            print("Only right sensor is on")
        elif(content == commands[botId][2]):
            print("Left white sensor is on")    
        elif(content ==commands[botId][4]):
            print("Bot Is Busy")
        elif(content == commands[botId][3]):
            """
            This means that all three sensors are on. This means we have reached some node. It can be either an intermediate node or a destination mode.
            """
            if(len(self.path) != 0):
                """
                This means we have not reached the destination yet.
                """
                if not self.get_collision_status(self.path[0][0], self.path[0][1]):
                    """
                    This is Collision check. What it does bot is at some node currently and bot knows in which direction it has to move. So it knows it's next node. It just checks that is there any other bot whose next node is same or not. If there is no other bot with same next node then it will go ahead but if there is some other bot then it will stay at current position till other bot's next position changes 
                    """
                    #send command
                    sent = self.xbee.SendStr(commands[botId][self.commands[0]])
                    print "BotId ", self.id, " sending ", commands[botId][self.commands[0]]
                    """
                    sends command to move and updating its next node's coordinates
                    """
                    self.nextX = self.path[0][0]
                    self.nextY = self.path[0][1]
                    self.update_next_pos()
                    #delete path
                    self.path.pop(0)
                    print self.path
                    #delete command
                    self.commands.pop(0)
            else:
                """
                This means bot reached it's destination and sends different commands for serving left food or right food according to order_position
                """
                if self.order_position==1:
                    sent = self.xbee.SendStr(commands[botId][4])
                    print("Serving left Food")
                elif self.order_position==2:
                    sent = self.xbee.SendStr(commands[botId][5])
                    print("Serving right Food")
                self.order_delivered()
        elif(content == commands[botId][5]):
            """
            this is stop state when bot is at start position and no order to serve
            """
            print("Bot in stop state")
            self.set_status(0)
        sleep(2)

    """
    Function Name : move
    Input :  Nothing
    Output : Nothing
    Logic : Send signal get_status to get the current state of bot
            If bot is in status 1
            Busy = Do nothing
            Stopped = Do nothing
            Left_Sensor_On = Do nothing
            Right_Sensor_On = Do nothing
            All three on = Send next command if next position not in use

    Example Call : move()
    """

    def move(self):
        """
        
        """
        if(self.status == 1):
            botId = self.id-1
            Msg = self.send_status_signal()
            if Msg:
                self.take_action(Msg)

    #def assign_orders(self):
        # Server
    """
    Function Name : set_status
    Input :  integer
    Output : Nothing
    Logic : Just sets the status variable of the class {0 : not moving, 1: moving}
    Example Call : set_status(1)
    """
    def set_status(self, status):
        # Calls the toggle command on bot
        print "Status of bot ",self.id,status
        self.status = status
        # Server

    """
    Function Name : check_base
    Input :  Nothing
    Output : Bot is at base or not
    Logic : Just compares the current position and base position of the bot
    Example Call : check_base()
    """

    def check_base(self):
        print self.X, self.Y
        if self.X == self.baseX and self.Y == self.baseY:
            return True
        return False

    def assign(self):
        self.assign_table()
        self.assign_base()

    """
    Function Name : assign_table
    Input :  Nothing
    Output : Assign a table to deliver if bot is free and unassigned order exist
    Logic : First it checks that bot is free or not. If not then do nothing. Else it gets the list of all unassigned orders for that bot from our webpage. And if there is any unassigned order exist then we set the destination of the bot according to that order.
    Example Call : assign_table()
    """

    def assign_table(self):
        if(self.status==0 and self.check_base()):
            unassigned_orders = json.loads(requests.get('http://127.0.0.1:8000/cs308_restaurant_app/nondelivered_orders/'+str(self.id)).content)
            print unassigned_orders
            arr=unassigned_orders['nondelivered_orders']
            if len(arr)>0:
                self.set_destination(tableX[arr[0]['table_num']], tableY[arr[0]['table_num']], arr[0]['order_pos'])
                self.set_status(1)
                self.order_id = arr[0]['id']

    """
    Function Name : assign_base
    Input :  Nothing
    Output : Assign destination as base after delivering the order
    Example Call : assign_base()

    """

    def assign_base(self):
        if(self.status==0 and not self.check_base()):
            self.set_destination(self.baseX,self.baseY, 1)
            self.set_status(1)

    """
    Function Name : order_delivered
    Input :  Nothing
    Output : Update entry in web app after delivering the order
    Example Call : order_delivered()
    
    """

    def order_delivered(self):
        return requests.get('http://127.0.0.1:8000/cs308_restaurant_app/order_delivered/'+str(self.order_id)).content

    """
    Function Name : update_next_pos
    Input :  Nothing
    Output : Update next positions in the web app to detect collision detection
    Example Call : update_next_pos()
    
    """

    def update_next_pos(self):
        self.X = self.nextX
        self.Y = self.nextY
        return requests.get('http://127.0.0.1:8000/cs308_restaurant_app/change_bot_pos/'+str(self.id)+"/"+str(self.nextX)+"/"+str(self.nextY)).content        

    """
    Function Name : get_collision_status
    Input :  a position (x and y coordinates)
    Output : True => collision , False => No collision
    Logic : Input is next position for this bot. It gets collision status from the web app as web app has positions of all other bots also and return according to that.
    Example Call : get_collision_status(2,2)
    
    """

    def get_collision_status(self, nextX, nextY):
        data = int(requests.get('http://127.0.0.1:8000/cs308_restaurant_app/get_collision_status/'+str(nextX)+"/"+str(nextY)).content)
        print "Collision status for ", self.id, nextX, nextY
        print data
        if data==0:
            return False
        return True

"""
Class Name - xbeeCommunicator()
This class is for communicating between xbee and bots. And this class runs both bots simultaneously also. 
"""

class xbeeCommunicator():
    """
    Function Name : __init__
    Logic : It is the intialisation of the class xbeeCommunicator. Here we store -
            xbee : port which is used for communication
            bots : It is an array where every element is a bot with botId and it's base position

    """
    def __init__(self):
        PORT = '/dev/ttyUSB0'
        self.xbee = XBee.XBee(PORT)  # Your serial port name here
        self.bots = [Bot(self,1,1,0),Bot(self,2,2,0)]

    """
    Function that delivers the food after bot reached the destination according to the direction in 
    which the bot is moving and side in which food is kept on the bot
    """

    
    def SendStr(self,char):
        return self.xbee.SendStr(char)

    def Receive(self):
        return self.xbee.Receive()
    
    """
    Function Name : start_communicating
    Logic : It is the main function which is called once only. It calls the assign and move function for both bots one by one so that they move simultaneously.
            
    """
    def start_communicating(self):
        while(True):
            for bot_num in range(2):
                self.bots[bot_num].assign()
                self.bots[bot_num].move()

xbeeCommunicatorObject = xbeeCommunicator()
xbeeCommunicatorObject.start_communicating()
