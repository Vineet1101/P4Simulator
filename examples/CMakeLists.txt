build_lib_example(
  NAME p4-ipv4-forwarding-refactor
  SOURCE_FILES p4-ipv4-forwarding-refactor.cc
  LIBRARIES_TO_LINK
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
    ${libcsma}
)

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
