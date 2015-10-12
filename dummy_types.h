#ifndef D_DUMMY_TYPES_H
#define D_DUMMY_TYPES_H

#include <cstdint>

namespace FTS {
    class Packet;
}

namespace FTSSrv2 {

class Graphic {
private:
    uint16_t m_usW;
    uint16_t m_usH;
    int8_t m_cForceAniso;
    int8_t m_cForceFilter;
    uint8_t *m_pData;

public:
    Graphic() {m_usW = m_usH = 0; m_cForceAniso = m_cForceFilter = 0; m_pData = 0;};
    virtual ~Graphic() { m_usW = m_usH = 0; m_cForceAniso = m_cForceFilter = 0; delete m_pData; m_pData = nullptr; };

    int writeToPacket(FTS::Packet *in_pPacket) const;
    int readFromPacket(FTS::Packet *in_pPacket);
};

} // namespace FTSSrv2

#endif /* D_DUMMY_TYPES_H */

 /* EOF */
