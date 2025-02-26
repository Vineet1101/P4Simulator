
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



