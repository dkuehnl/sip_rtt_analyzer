# sip_rtt_analyzer
An easy cli-tool for generating sip-options and analyzing the round-trip-time (time between sending option and receiving a sip-response).

# functions
-c  with this option you can set a custom value for how many requests should be sent.       
    Default-value is 5.

--send-summery=true: causes the tool to send a summery-request inlcuded in the last option-request. default is false

-d  Defines the destination-ip-adress for the sip-proxy against which the options should be sent. Mandatory option

-e  Oportunity for exporting the the results as an .csv-file