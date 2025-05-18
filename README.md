# node_mcu_android_app
control node mcu with android app, wifi and hotspot, without inetrnet
Create the android app by mit
 url_offline_led_ajax    = automatic show the led status
 url_offline_led_both_mode = normal UI just connect to the nodemcu hotspot and http://192.168.4.1 or http://ctrl.me
 url_offline_led_both_mode_Admin = It is dual mode, we can change the wifi, Steps:- Restart the nodemcu --> connnect phone wifi to ESP-LED-Config --> go to browser 
            http://192.168.4.1   --> scan wifi, give credential and connect  
            Restart the nodeMCU  --> connnect phone wifi to ESP-LED-AP       
            http://192.168.4.1/admin or http://ctrl.me/admin   --> get the IP of NodeMCU

ir_receiver-admin3_nodemcu.ino = extra feature we can edit the hexcode accoding to the Ledpin on http://ctrl.me/ir-admin
    
     
