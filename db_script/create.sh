#! /bin/bash

mysql -h10.0.2.15 -uznf -p123 < db1_profile.sql
mysql -h10.0.2.15 -uznf -p123 < db2_user.sql
