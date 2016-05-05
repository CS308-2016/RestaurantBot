from django.conf.urls import url
from . import views

urlpatterns = [
	url(r'^place_order/',views.place_order,name = 'placing_order'),
	url(r'^cooking_food/',views.cook_food,name = 'cooking_food'),
	url(r'^orders_left/',views.orders_left,name = 'orders_left'),
	url(r'^pending_orders/(?P<table_num>\d+)',views.pending_orders_table, name='pending_orders_table'),
	url(r'^nondelivered_orders/(?P<bot_num>\d+)',views.nondelivered_orders,name = 'nondelivered_orders'),
	url(r'^order_delivered/(?P<order_id>\d+)',views.order_delivered, name='order_delivered'),
	url(r'^change_bot_pos/(?P<bot_id>\d+)/(?P<num_x>\d+)/(?P<num_y>\d+)',views.change_bot_pos, name='change_bot_pos'),
	url(r'^change_bot_status/(?P<bot_id>\d+)/(?P<status>\d+)',views.change_bot_status, name='change_bot_status'),
	url(r'^get_collision_status/(?P<num_x>\d+)/(?P<num_y>\d+)',views.get_collision_status, name='get_collision_status'),
]
