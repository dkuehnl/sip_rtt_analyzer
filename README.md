# sip_rtt_analyzer

An easy cli-tool for generating SIP-options and analyzing the round-trip-time (time between sending option and receiving a sip-response).

# USAGE
siprta -d [IP-Addr] [OPTION][VALUE]

# FUNCTIONS
-c,                     Number of requests to be sent (default: 5)
-u,                     Activates UDP (default: TCP)
-s,                     Set the sleep-timer between every paket (default: 500ms)
                        Becareful: unit is mikroseconds
*-d,                    Destination IP-address of SIP-proxy (format: xxx.xxx.xxx.xxx)
-p,                     Destination port for the requests (default: 5060)
--send-summary[=true]   If set to 'true' the summary will be sent as a separate     OPTION-Request to the proxy. Every requests has it's own X-Header with varios information. Only the first ten pakets will be send. Default-value: false
--version               Show version
--help                  Show help
                        Attention: there is no explicit implemented security-mechanism to protect the sip-proxy.

# ATTENTION
The user have the full responsibility to not overload the sip-proxy.
=======
An easy cli-tool for generating sip-options and analyzing the round-trip-time (time between sending option and receiving a sip-response).

# functions
-c  with this option you can set a custom value for how many requests should be sent.       
    Default-value is 5.

--send-summery=true: causes the tool to send a summery-request inlcuded in the last option-request. default is false

-d  Defines the destination-ip-adress for the sip-proxy against which the options should be sent. Mandatory option

-e  Oportunity for exporting the the results as an .csv-file

