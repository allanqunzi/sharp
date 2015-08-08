#!/bin/bash

cd /home/qunzi/Downloads/unixmacosx_latest/IBJts
java -cp jts.jar:total.2013.jar -Xmx768M -XX:MaxPermSize=256M jclient.LoginFrame .
