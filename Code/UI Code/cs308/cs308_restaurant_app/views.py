from django.shortcuts import render, redirect
from django.views.decorators.csrf import csrf_exempt

from django.http import HttpResponse
from models import Orders, BotStatus, BotTableMatch
from forms import orderForm
from django.db.models import Q

from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework.renderers import JSONRenderer

import cgi
import restaurant_info
import simplejson as json
import main

# Create your views here.
"""
Class : JSONResponse
Logic : An HttpResponse that renders its content into JSON.
"""
class JSONResponse(HttpResponse):
   
    def __init__(self, data, **kwargs):
        content = JSONRenderer().render(data)
        kwargs['content_type'] = 'application/json'
        super(JSONResponse, self).__init__(content, **kwargs)

"""
Function Name : place_order
Input : HTTPRequest ( 2 types of request 'GET' and 'POST')
	GET - Return the webpage of placing order
	POST - take the table number and dish number from the form. First it checks that these entries are valid or not. If entries are valid then it save these entries else render the webpage of placing order again
"""

def place_order(request):
	"""
	This shows the form of order placement and posts it if filled form received
	"""
	if request.method=='GET':
		data={}
		data['num_tables'] = range(restaurant_info.NUM_TABLES)
		data['num_dishes'] = range(restaurant_info.NUM_DISHES)
		return render(request, 'place_order.html', data)
	elif request.method=='POST':
		data = request.POST
		form = orderForm(data)
		if form.is_valid():
			form.save()
		else:
			data={}
			data['num_tables'] = range(restaurant_info.NUM_TABLES)
			data['num_dishes'] = range(restaurant_info.NUM_DISHES)
			return render(request, 'place_order.html', data)

		return redirect('pending_orders_table',table_num=data['table_num'])
		
"""
Function Name : orders_left
Input : HTTPRequest ( 2 types of request 'GET' and 'POST')
	GET - It checks all orders with order status as 0 (pending) and render the 'pending.html'
		  In this web page it shows a form in which an order can be assigned to a bot.
	POST - post request means form is filled for some order. so first the status of that order is changed to 1 as it has been assigned to some bot. And after it that bot is assigned the order to deliver it. 
"""

@csrf_exempt
def orders_left(request):
	"""
	This page shows the orders left and places the order if it is made
	"""
	if request.method=="GET":
		data = {}
		data['orders_left'] = Orders.objects.filter(Q(order_status=0) | Q(bottablematch=None))
		data['num_bots'] = range(restaurant_info.NUM_BOTS)
		data['num_positions'] = range(restaurant_info.NUM_POSITIONS)
		print(data)
		return render(request,'pending.html',data)
	elif request.method=="POST":
		data = request.POST
		order = Orders.objects.get(pk=data['order_id'])
		order.order_status=1
		print data
		try:
			bot = BotStatus.objects.get(pk=data['num_bot'])
		except BotStatus.DoesNotExist:
			bot = BotStatus(bot_num=data['num_bot'],next_x=0,next_y=0)
			bot.save()
		"""
		If bot is not free then return some error else save the order and then put an entry in BotTableMatch that bot has been assigned this order and position of the food on the bot.
		"""
		if not bot.bot_status:
			order.save()
			bottablematchobject = BotTableMatch(bot_num=bot, order_num=order, order_position=data['num_pos'])
			bottablematchobject.save()
			result = {}
			result['message'] = "Done"
			return HttpResponse(json.dumps(result))
		else:
			return HttpResponse(status=500)

"""
Function Name : pending_orders_table
Input : HTTPRequest and a table number
output : It just tells all orders of a given table number that is still pending.
			Pending means that are not cooked by shef till now.
			And render the webpage 'orders_left_table.html' according to that
"""

def pending_orders_table(request, table_num):
	"""
	Shows the pending orders for a particular table number
	"""
	data = {}
	data['orders_left'] = Orders.objects.filter(order_status=0,table_num=table_num)
	data['table_num'] = table_num
	return render(request,'orders_left_table.html',data)

"""
Function Name : nondelivered_orders
Input : HTTPRequest and a bot number
	GET - It takes the all entries of the table BotTableMatch corresponding to a given bot number and delivery_status as 0 means not delivered yet.
		And it returns all those entries in HttpResponse 
"""

@csrf_exempt
@api_view(['GET'])
def nondelivered_orders(request, bot_num):
	"""
	Returns json of orders assigned not delivered
	"""
	if request.method=="GET":
		data = BotTableMatch.objects.filter(bot_num=bot_num,delivery_status=0)
		print data
		orders = []
		for datum in data:
			order = {}
			order["table_num"] = datum.order_num.table_num
			order["dish_num"] = datum.order_num.dish_num
			order["order_pos"] = datum.order_position
			order["id"] = datum.id
			orders.append(order)
		data = {}
		data['nondelivered_orders'] = orders
		return Response(data)

"""
Function Name : order_delivered
Input : HTTPRequest and an order id (unique for every order)
	GET - it just updates the entry corresponding to order id in BotTableMatch table that the order is delivered and make delivery_status = 1
"""

@api_view(['GET'])
def order_delivered(request, order_id):
	"""
	Sets the delivery status of bottablematch to 1
	"""
	if request.method=="GET":
		bottablematchobject = BotTableMatch.objects.get(pk=order_id)
		bottablematchobject.delivery_status = 1
		bottablematchobject.save()
		return Response("Done")

"""
Function Name : change_bot_pos
Input : HTTPRequest, a bot id and a position (x and y coordinates)
	GET - it just updates the position corresponding to the bot id in BotStatus table 
"""

@api_view(['GET'])
def change_bot_pos(request, bot_id, num_x, num_y):
	"""
	Changes the position of bot given by bot_id to num_x and num_y
	"""
	if request.method=="GET":
		bot = BotStatus.objects.get(pk=bot_id)
		bot.next_x = num_x
		bot.next_y = num_y
		bot.save()
		return Response("Done")

"""
Function Name : change_bot_status
Input : HTTPRequest, a bot id and a status
	GET - it just updates the status corresponding to the bot id in BotStatus table 
"""

@api_view(['GET'])
def change_bot_status(request, bot_id, status):
	"""
	Changes status of bot given by bot_id
	"""
	if request.method=="GET":
		bot = BotStatus.objects.get(pk=bot_id)
		bot.bot_status = status
		bot.save()
		return Response("Done")

"""
Function Name : get_collision_status
Input : HTTPRequest, a position (x and y coordinates)
	GET - Checks if collision is possible given x and y for the bot
		  0 if not collision
		  1 if collision
"""

@api_view(['GET'])
def get_collision_status(request, num_x, num_y):
	if request.method=="GET":
		bot = BotStatus.objects.filter(next_x=num_x, next_y=num_y)
		if not bot:
			return Response(0)
		return Response(1)
