#include "../djk_ipv4/gia_djk.h"
#include <vector>

vector<linkAdjStr_t> adjVec {
        {"10.0.0.1", "10.254.241.2", "255.255.255.252", 5, "10.254.241.1", "172.16.0.1"}, // S
        {"10.0.0.1", "10.254.241.9", "255.255.255.252", 10, "10.254.241.10", "192.168.168.7"},
        {"10.0.0.1", "10.254.241.49", "255.255.255.248", 11, "10.254.241.50", "10.0.0.3"}, // equal-cost link to C
        {"10.0.0.1", "10.254.241.45", "255.255.255.252", 11, "10.254.241.46", "10.0.0.3"}, // equal-cost link to C
        {"10.0.0.1", "10.254.241.49", "255.255.255.248", 11, "10.254.241.51", "10.0.0.4"},  // equal-cost link
        {"10.0.0.1", "10.0.0.1", "255.255.255.255", 0, "10.0.0.1", "10.0.0.1"},  // loopback

        {"172.16.0.1","10.254.241.1", "255.255.255.252", 5, "10.254.241.2", "10.0.0.1"}, // A
        {"172.16.0.1","10.254.241.5", "255.255.255.252", 16, "10.254.241.6", "192.168.0.5"},
        {"172.16.0.1","8.8.8.2", "255.255.255.0", 17, "0.0.0.0", "0.0.0.0"}, // stub (no neighbor)
        {"172.16.0.1", "172.16.0.1", "255.255.255.255", 0, "172.16.0.1", "172.16.0.1"},  // loopback

        {"192.168.168.7","10.254.241.10", "255.255.255.252", 10, "10.254.241.9", "10.0.0.1"}, // B
        {"192.168.168.7","10.254.241.17", "255.255.255.252", 2, "10.254.241.18", "10.0.0.3"},
        {"192.168.168.7","10.254.241.21", "255.255.255.252", 8, "10.254.241.22", "172.16.172.10"},
        {"192.168.168.7","10.254.241.29", "255.255.255.252", 20, "10.254.241.30", "192.168.0.5"},
        {"192.168.168.7","192.168.168.7", "255.255.255.255", 0, "192.168.168.7", "192.168.168.7"}, // loopback

        {"10.0.0.3","10.254.241.50", "255.255.255.252", 10, "10.254.241.49", "10.0.0.1"}, // C
        {"10.0.0.3","10.254.241.50", "255.255.255.252", 10, "10.254.241.51", "10.0.0.4"}, //
        {"10.0.0.3","10.254.241.46", "255.255.255.252", 11, "10.254.241.45", "10.0.0.1"}, // non qual-cost parallel link to S
        {"10.0.0.3","10.254.241.18", "255.255.255.252", 2, "10.254.241.17", "192.168.168.7"},
        {"10.0.0.3","10.254.241.25", "255.255.255.252", 1, "10.254.241.26", "172.16.172.10"},
        {"10.0.0.3","10.0.0.3", "255.255.255.255", 0, "10.0.0.3", "10.0.0.3"}, // loopback

        {"192.168.0.5","10.254.241.6", "255.255.255.252", 16, "10.254.241.5", "172.16.0.1"}, // D
        {"192.168.0.5","10.254.241.30", "255.255.255.252", 20, "10.254.241.29", "192.168.168.7"},
        {"192.168.0.5","10.254.241.33", "255.255.255.252", 1, "10.254.241.34", "172.16.172.10"},
        {"192.168.0.5","10.254.241.37", "255.255.255.252", 8, "10.254.241.38", "10.0.0.2"},
        {"192.168.0.5","192.168.0.5", "255.255.255.255", 0, "192.168.0.5", "192.168.0.5"}, // loopback

        {"172.16.172.10","10.254.241.22", "255.255.255.252", 8, "10.254.241.21", "192.168.168.7"}, // E
        {"172.16.172.10","10.254.241.26", "255.255.255.252", 1, "10.254.241.25", "10.0.0.3"},
        {"172.16.172.10","10.254.241.34", "255.255.255.252", 1, "10.254.241.33", "192.168.0.5"},
        {"172.16.172.10","10.254.241.41", "255.255.255.252", 12, "10.254.241.42", "10.0.0.2"},
        {"172.16.172.10","10.254.241.14", "255.255.255.252", 1, "10.254.241.13", "10.0.0.4"},
        {"172.16.172.10","172.16.172.10", "255.255.255.255", 0, "172.16.172.10", "172.16.172.10"}, // loopback

        {"10.0.0.2","10.254.241.38", "255.255.255.252", 8, "10.254.241.37", "192.168.0.5"}, // F
        {"10.0.0.2","10.254.241.42", "255.255.255.252", 12, "10.254.241.41", "172.16.172.10"},
        {"10.0.0.2","10.254.241.53", "255.255.255.252", 8, "10.254.241.54", "172.16.172.66"},
        {"10.0.0.2","8.8.8.1", "255.255.255.0", 1, "0.0.0.0", "0.0.0.0"}, // stub (no neighbor)
        {"10.0.0.2","10.0.0.2", "255.255.255.255", 0, "10.0.0.2", "10.0.0.2"}, // loopback

        {"172.16.172.66","10.254.241.54", "255.255.255.252", 12, "10.254.241.53", "10.0.0.2"}, // X
        {"172.16.172.66","172.16.172.66", "255.255.255.255", 0, "172.16.172.66", "172.16.172.66"}, // loopback

        {"10.0.0.4","10.254.241.51", "255.255.255.248", 10, "10.254.241.49", "10.0.0.1"}, // G
        {"10.0.0.4","10.254.241.13", "255.255.255.252", 1, "10.254.241.14", "172.16.172.10"},
        {"10.0.0.4","10.254.241.51", "255.255.255.252", 10, "10.254.241.50", "10.0.0.3"},
        {"10.0.0.4","10.0.0.4", "255.255.255.255", 0, "10.0.0.4", "10.0.0.4"}, // loopback

    };

