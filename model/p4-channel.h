/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This implementation uses a customized version of the Point-to-Point Channel 
 * to integrate with the P4-Net-Device. The reason for copying and modifying the 
 * Point-to-Point Channel code is that the default Point-to-Point Net Device 
 * does not allow overriding its receive and processing functions, which are 
 * essential for P4-based packet handling. Additionally, the TransmitStart 
 * function in the original Point-to-Point Channel cannot be overridden via 
 * inheritance. 
 *
 * To address these limitations, the Point-to-Point Channel code was replicated 
 * and adjusted to allow the necessary customizations while preserving the 
 * original logic and behavior of the channel. This ensures compatibility with 
 * P4-Net-Device without introducing major changes to the core design.
 * 
 */


#ifndef P4_CHANNEL_H
#define P4_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <list>

namespace ns3
{

class P4NetDevice;
class Packet;

/**
 * \ingroup point-to-point for P4 network devices
 * \brief Simple Point To Point Channel.
 *
 * This class is same from point-to-point-channel.h, but it is used for P4 network devices.
 * 
 * This class represents a very simple point to point channel.  Think full
 * duplex RS-232 or RS-422 with null modem and no handshaking.  There is no
 * multi-drop capability on this channel -- there can be a maximum of two
 * point-to-point net devices connected.
 *
 * There are two "wires" in the channel.  The first device connected gets the
 * [0] wire to transmit on.  The second device gets the [1] wire.  There is a
 * state (IDLE, TRANSMITTING) associated with each wire.
 *
 * \see Attach
 * \see TransmitStart
 */
class P4Channel : public Channel
{
  public:
    /**
     * \brief Get the TypeId
     *
     * \return The TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * \brief Create a P4Channel
     *
     * By default, you get a channel that has an "infinitely" fast
     * transmission speed and zero delay.
     */
    P4Channel();

    /**
     * \brief Attach a given netdevice to this channel
     * \param device pointer to the netdevice to attach to the channel
     */
    void Attach(Ptr<P4NetDevice> device);

    /**
     * \brief Transmit a packet over this channel
     * \param p Packet to transmit
     * \param src Source P4NetDevice
     * \param txTime Transmit time to apply
     * \returns true if successful (currently always true)
     */
    virtual bool TransmitStart(Ptr<const Packet> p, Ptr<P4NetDevice> src, Time txTime);

    /**
     * \brief Get number of devices on this channel
     * \returns number of devices on this channel
     */
    std::size_t GetNDevices() const override;

    /**
     * \brief Get P4NetDevice corresponding to index i on this channel
     * \param i Index number of the device requested
     * \returns Ptr to P4NetDevice requested
     */
    Ptr<P4NetDevice> GetPointToPointDevice(std::size_t i) const;

    /**
     * \brief Get NetDevice corresponding to index i on this channel
     * \param i Index number of the device requested
     * \returns Ptr to NetDevice requested
     */
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

  protected:
    /**
     * \brief Get the delay associated with this channel
     * \returns Time delay
     */
    Time GetDelay() const;

    /**
     * \brief Check to make sure the link is initialized
     * \returns true if initialized, asserts otherwise
     */
    bool IsInitialized() const;

    /**
     * \brief Get the net-device source
     * \param i the link requested
     * \returns Ptr to P4NetDevice source for the
     * specified link
     */
    Ptr<P4NetDevice> GetSource(uint32_t i) const;

    /**
     * \brief Get the net-device destination
     * \param i the link requested
     * \returns Ptr to P4NetDevice destination for
     * the specified link
     */
    Ptr<P4NetDevice> GetDestination(uint32_t i) const;

    /**
     * TracedCallback signature for packet transmission animation events.
     *
     * \param [in] packet The packet being transmitted.
     * \param [in] txDevice the TransmitTing NetDevice.
     * \param [in] rxDevice the Receiving NetDevice.
     * \param [in] duration The amount of time to transmit the packet.
     * \param [in] lastBitTime Last bit receive time (relative to now)
     * \deprecated The non-const \c Ptr<NetDevice> argument is deprecated
     * and will be changed to \c Ptr<const NetDevice> in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    typedef void (*TxRxAnimationCallback)(Ptr<const Packet> packet,
                                          Ptr<NetDevice> txDevice,
                                          Ptr<NetDevice> rxDevice,
                                          Time duration,
                                          Time lastBitTime);

  private:
    /** Each point to point link has exactly two net devices. */
    static const std::size_t N_DEVICES = 2;

    Time m_delay;           //!< Propagation delay
    std::size_t m_nDevices; //!< Devices of this channel

    /**
     * The trace source for the packet transmission animation events that the
     * device can fire.
     * Arguments to the callback are the packet, transmitting
     * net device, receiving net device, transmission time and
     * packet receipt time.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<NetDevice> argument is deprecated
     * and will be changed to \c Ptr<const NetDevice> in a future release.
     */
    // NS_DEPRECATED() - tag for future removal
    TracedCallback<Ptr<const Packet>, // Packet being transmitted
                   Ptr<NetDevice>,    // Transmitting NetDevice
                   Ptr<NetDevice>,    // Receiving NetDevice
                   Time,              // Amount of time to transmit the pkt
                   Time               // Last bit receive time (relative to now)
                   >
        m_txrxPointToPoint;

    /** \brief Wire states
     *
     */
    enum WireState
    {
        /** Initializing state */
        INITIALIZING,
        /** Idle state (no transmission from NetDevice) */
        IDLE,
        /** Transmitting state (data being transmitted from NetDevice. */
        TRANSMITTING,
        /** Propagating state (data is being propagated in the channel. */
        PROPAGATING
    };

    /**
     * \brief Wire model for the P4Channel
     */
    class Link
    {
      public:
        /** \brief Create the link, it will be in INITIALIZING state
         *
         */
        Link() = default;

        WireState m_state{INITIALIZING};  //!< State of the link
        Ptr<P4NetDevice> m_src; //!< First NetDevice
        Ptr<P4NetDevice> m_dst; //!< Second NetDevice
    };

    Link m_link[N_DEVICES]; //!< Link model
};

} // namespace ns3

#endif /* P4_CHANNEL_H */
