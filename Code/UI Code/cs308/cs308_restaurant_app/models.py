from __future__ import unicode_literals

from django.db import models
from django.core.validators import MaxValueValidator, MinValueValidator
import restaurant_info

"""
Order class to track the orders placed by table and the status if they have been made or not
This represents all classes that are used in our web application
"""

"""
Class : Orders
Stores : It stores a single order of a table. So it stores ->
	table_num - Table from which the order is given
	dish_num - Type of the dish ordered
	order_status - A boolean field whether the order has been given to the bot or not
	created_at - Date time field to store the date and time at which the order is given
"""
class Orders(models.Model):
	table_num = models.IntegerField(validators=[MinValueValidator(1), MaxValueValidator(restaurant_info.NUM_TABLES)])
	dish_num = models.IntegerField(validators=[MinValueValidator(1), MaxValueValidator(restaurant_info.NUM_DISHES)])
	order_status = models.BooleanField(default=False)
	created_at = models.DateTimeField(auto_now_add=True)

"""

Class : BotStatus
Stores : It stores the complete status of a bot. So it stores ->
	bot_num - A unique number given to every bot
	bot_status - denotes if bot is free or not (0 for free, 1 for busy)
	next_x - next x coordinate of bot
	next_y - next y coordinate of bot

"""
class BotStatus(models.Model):
	bot_num = models.IntegerField(validators=[MinValueValidator(1),MaxValueValidator(restaurant_info.NUM_BOTS)],primary_key=True)
	bot_status = models.BooleanField(default=False)
	next_x = models.IntegerField()
	next_y = models.IntegerField()

"""

Class : BotTableMatch
Stores : It stores the what table has been assigned to what bot. So it stores ->
	bot_num - The number of the bot which is taking the food to deliver
	order_num - type of the dish being served
	order_position - position of order on the bot {1 for left and 2 for right}
	delivery_status - it denotes that we have delivered the food or not
	assigned_at - the date and time at which bot is assigned to deliver
	delivered_at - the date and time at which food is delivered by bot

	To assign delivered_at properly save the Model once after delivery
	
"""
class BotTableMatch(models.Model):
	bot_num = models.ForeignKey(BotStatus, on_delete=models.CASCADE)
	order_num = models.ForeignKey(Orders, on_delete=models.CASCADE)
	order_position = models.IntegerField(validators=[MinValueValidator(1),MaxValueValidator(2)])
	delivery_status = models.BooleanField(default=False)
	assigned_at = models.DateTimeField(auto_now_add = True)
	delivered_at = models.DateTimeField(auto_now=True)
