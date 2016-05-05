from django import forms
from models import *

class orderForm(forms.ModelForm):
	class Meta:
		model = Orders
		fields = ('table_num','dish_num')