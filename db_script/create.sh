#! /bin/bash

mysql -hlocalhost -uroot < db1_profile.sql
mysql -hlocalhost -uroot < db2_user.sql
