#include "dummy_types.h"

#include <packet.h>

using namespace FTS;
using namespace FTSSrv2;

int FTSSrv2::Graphic::writeToPacket(Packet *in_pPacket) const
{
    // Write informations to the packet.
    in_pPacket->append(m_usW);
    in_pPacket->append(m_usH);
    in_pPacket->append(m_cForceAniso);
    in_pPacket->append(m_cForceFilter);

    // Write data to the packet.
    in_pPacket->append(m_pData, m_usH*m_usW*4);

    return 0;
}

int FTSSrv2::Graphic::readFromPacket(Packet *in_pPacket)
{
    m_usW = m_usH = 0;
    m_cForceAniso = m_cForceFilter = 0;
    delete m_pData;
    m_pData = nullptr;

    // Read the informations from the packet.
    in_pPacket->get(m_usW);
    in_pPacket->get(m_usH);
    in_pPacket->get(m_cForceAniso);
    in_pPacket->get(m_cForceFilter);

    if(m_usW*m_usH <= 4096*4096) {
        m_pData = (uint8_t *)calloc(m_usW*m_usH*4, sizeof(uint8_t));
        if(m_pData != NULL) {
            // Read the data from the packet.
            in_pPacket->get(m_pData, m_usH*m_usW*4);
        }
    }

    return 0;
}
