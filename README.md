# sip_rtt_analyzer
An easy cli-tool for generating sip-options and analyzing the round-trip-time (time between sending option and receiving a sip-response).

# USAGE
siprta -d [IP-Addr] [OPTION][VALUE]

# FUNCTIONS
-c,                     Number of requests to be sent (default: 5)
-u,                     Activates udp (default: TCP)
-s,                     Set the sleep-timer between every paket (default: 500ms)
                        Becareful: unit is mikroseconds
*-d,                    Destination ip-address of sip-proxy (format: xxx.xxx.xxx.xxx)
-p,                     Destination port for the requests (default: 5060)
--send-summary[=true]   If set to 'true' the summary will be sent as a separate     OPTION-Request to the proxy. Every requests has it's own X-Header with varios information.
Only the first ten pakets will be send. For more information see the man-page. 
Default-value: false

--version               Show version
--help                  Show help
                        Attention: there is no explicit implemented security-mechanism to protect the sip-proxy.

# ATTENTION
The user have the full responsibility to not overload the sip-proxy.