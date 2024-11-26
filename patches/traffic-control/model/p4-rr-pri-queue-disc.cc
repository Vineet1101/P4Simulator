#include "p4-rr-pri-queue-disc.h"

#include "priority-port-tag.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NSP4PriQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(NSP4PriQueueDisc);

TypeId
NSP4PriQueueDisc::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::NSP4PriQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<NSP4PriQueueDisc>()
            .AddAttribute("NumPorts",
                          "Number of ports to manage.",
                          UintegerValue(4),
                          MakeUintegerAccessor(&NSP4PriQueueDisc::m_nbPorts),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("NumPriorities",
                          "Number of virtual queue to manage for each port (default 8).",
                          UintegerValue(8),
                          MakeUintegerAccessor(&NSP4PriQueueDisc::m_nbPriorities),
                          MakeUintegerChecker<uint8_t>());

    return tid;
}

NSP4PriQueueDisc::NSP4PriQueueDisc()
    : m_rng(CreateObject<UniformRandomVariable>())
{
    // init the priority queues with port vector
    m_priorityQueues.reserve(m_nbPriorities);
    for (uint8_t i = 0; i < m_nbPriorities; ++i)
    {
        m_priorityQueues.emplace_back(m_nbPorts);
    }

    // Initialize the random number generator range
    m_rng->SetAttribute("Min", DoubleValue(0));
    m_rng->SetAttribute("Max", DoubleValue(m_nbPorts - 1));

    NS_LOG_DEBUG("NSP4PriQueueDisc default constructor with " << m_nbPorts << " ports and "
                                                              << m_nbPriorities << " priorities");
}

NSP4PriQueueDisc::~NSP4PriQueueDisc()
{
}

uint32_t
NSP4PriQueueDisc::GetQueueSize(uint8_t port, uint8_t priority) const
{
    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
        NS_LOG_ERROR("Invalid port or priority specified in " << this);
        return 0;
    }
    return m_priorityQueues[port][priority].size;
}

uint32_t
NSP4PriQueueDisc::GetQueueCapacity(uint8_t port, uint8_t priority) const
{
    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
        NS_LOG_ERROR("Invalid port or priority specified in " << this);
        return 0;
    }
    return m_priorityQueues[port][priority].capacity;
}

uint64_t
NSP4PriQueueDisc::GetQueueRate(uint8_t port, uint8_t priority) const
{
    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
        NS_LOG_ERROR("Invalid port or priority specified in " << this);
        return 0;
    }
    return m_priorityQueues[port][priority].ratePps;
}

void
NSP4PriQueueDisc::SetQueueCapacity(uint8_t port, uint8_t priority, uint32_t capacity)
{
    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
        NS_LOG_ERROR("Invalid port or priority specified in " << this);
        return;
    }
    m_priorityQueues[port][priority].capacity = capacity;
}

void
NSP4PriQueueDisc::SetQueueRate(uint8_t port, uint8_t priority, uint64_t ratePps)
{
    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
        NS_LOG_ERROR("Invalid port or priority specified in " << this);
        return;
    }
    m_priorityQueues[port][priority].ratePps = ratePps;
    m_priorityQueues[port][priority].delayTime = RateToTime(ratePps);
}

bool
NSP4PriQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    Ptr<Packet> packet = item->GetPacket();
    PriorityPortTag tag;

    if (packet->PeekPacketTag(tag))
    {
        // get the priority and port from the tag
        uint8_t priority = tag.GetPriority() % m_nbPriorities;
        uint16_t port = tag.GetPort();

        if (port >= m_nbPorts || priority >= m_nbPriorities)
        {
            NS_LOG_ERROR("Invalid port or priority specified in " << this);
            return false;
        }

        auto& fifoQueue = m_priorityQueues[port][priority];
        if (fifoQueue.size >= fifoQueue.capacity)
        {
            NS_LOG_WARN("Queue overflow for port " << port << ", priority " << priority);
            DropBeforeEnqueue(item, LIMIT_EXCEEDED_DROP);
            return false;
        }

        Time sendTime = CalculateNextSendTime(fifoQueue);
        QueueElement queueElement(item, sendTime);
        fifoQueue.queue.push(queueElement);

        fifoQueue.size++;
        fifoQueue.queueLengthPort++;

        NS_LOG_INFO("Packet contains a PriorityPortTag with priority " << priority << " and port "
                                                                       << port);
    }

    else
    {
        NS_LOG_WARN("Packet does not contain a PriorityPortTag");
        return false;
    }
    return true;
}

uint8_t
NSP4PriQueueDisc::GetNextPort() const
{
    uint8_t selectedPort = static_cast<uint8_t>(m_rng->GetValue());
    NS_ASSERT(selectedPort < m_nbPorts);
    return selectedPort;
}

Ptr<QueueDiscItem>
NSP4PriQueueDisc::DoDequeue(void)
{
    uint8_t port = GetNextPort();

    for (uint8_t pri = 0; pri < m_nbPriorities; pri++)
    {
        auto& fifoQueue = m_priorityQueues[port][pri];
        if (fifoQueue.queue.empty())
        {
            continue;
        }

        Time now = Simulator::Now();
        if (fifoQueue.queue.front().sendTime >= now)
        {
            Ptr<QueueElement> dequeuedElement = fifoQueue.queue.front().item;
            fifoQueue.queue.pop();
            fifoQueue.size--;

            Ptr<QueueItem> dequeuedItem = dequeuedElement->item;

            return dequeuedItem;
        }
    }
    return nullptr;
}

Ptr<const QueueDiscItem>
NSP4PriQueueDisc::DoPeek(void) const
{
    uint8_t port = GetNextPort();

    for (uint8_t pri = 0; pri < m_nbPriorities; pri++)
    {
        auto& fifoQueue = m_priorityQueues[port][pri];
        if (fifoQueue.queue.empty())
        {
            continue;
        }

        Time now = Simulator::Now();
        if (fifoQueue.queue.front().sendTime >= now)
        {
            Ptr<QueueElement> peekElement = fifoQueue.queue.front().item;
            // fifoQueue.queue.pop ();
            // fifoQueue.size--;
            Ptr<QueueItem> peekItem = peekElement->item;

            return peekItem;
        }
    }
    return nullptr;
}

bool
NSP4PriQueueDisc::CheckConfig(void)
{
    if (m_priorityQueues.size() != m_nbPriorities)
    {
        NS_LOG_ERROR(
            "Number of priority queues for each port does not match the set number of priorities");
        return false;
    }
    return true;
}

Time
NSP4PriQueueDisc::CalculateNextSendTime(const FifoQueue& FifoQueue) const
{
    Time now = Simulator::Now();
    return now + FifoQueue.delayTime;
}

constexpr Time
NSP4PriQueueDisc::RateToTime(uint64_t pps)
{
    return (pps == 0) ? MilliSeconds(1) : Seconds(1.0 / pps);
}

void
NSP4PriQueueDisc::InitializeParams(void)
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
