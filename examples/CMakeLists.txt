# Evaluation example: p4-ipv4-forwarding-refactor
add_executable(p4-ipv4-forwarding-refactor p4-ipv4-forwarding-refactor.cc)
target_link_libraries(p4-ipv4-forwarding-refactor
    ${libp4sim}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
)