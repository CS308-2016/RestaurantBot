# -*- coding: utf-8 -*-
# Generated by Django 1.9.4 on 2016-03-23 08:10
from __future__ import unicode_literals

import django.core.validators
from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    dependencies = [
        ('cs308_restaurant_app', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='BotStatus',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('bot_num', models.IntegerField(validators=[django.core.validators.MinValueValidator(1), django.core.validators.MaxValueValidator(3)])),
                ('bot_status', models.BooleanField(default=False)),
            ],
        ),
        migrations.CreateModel(
            name='BotTableMatch',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('delivered_at', models.DateTimeField(auto_now_add=True)),
                ('bot_num', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='cs308_restaurant_app.BotStatus')),
                ('table_num', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='cs308_restaurant_app.Orders')),
            ],
        ),
    ]
