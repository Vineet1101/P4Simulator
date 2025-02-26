
# [ ================= V1model Arch ================= ]

# simple 2 hosts 1 switch topology [ipv4 forwarding]
build_lib_example(
  NAME p4-v1model-ipv4-forwarding
  SOURCE_FILES p4-v1model-ipv4-forwarding.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
    ${libcsma}
)

# 4 hosts 4 switches topology [ipv4 forwarding]
build_lib_example(
  NAME p4-basic-example
  SOURCE_FILES p4-basic-example.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
    ${libcsma}
)

# 3 hosts 3 switches topology [tunnel]
build_lib_example(
  NAME p4-basic-tunnel
  SOURCE_FILES p4-basic-tunnel.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
)

# [ ================= PSA Arch ================= ]

# simple 2 hosts 1 switch topology [ipv4 forwarding]
build_lib_example(
  NAME p4-psa-ipv4-forwarding
  SOURCE_FILES p4-psa-ipv4-forwarding.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
    ${libcsma}
)

# [ ================= NO P4 ================= ]

# simple 2 hosts sending with custom header [p2p]
build_lib_example(
  NAME p4-p2p-custom-header-test
  SOURCE_FILES p4-p2p-custom-header-test.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
)

# [ ================= Test ================= ]

# custom header test
build_lib_example(
  NAME p4-custom-header-test
  SOURCE_FILES p4-custom-header-test.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
)

