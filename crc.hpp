#pragma once

#include <cstdint>

namespace blobs
{

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

constexpr uint16_t crc16Ccitt = 0x1021;
/* Value from: http://srecord.sourceforge.net/crc16-ccitt.html for
 * implementation without explicit bit adding.
 */
constexpr uint16_t crc16Initial = 0xFFFF;

class CrcInterface
{
  public:
    virtual ~CrcInterface() = default;

    /**
     * Reset the crc.
     */
    virtual void clear() = 0;

    /**
     * Provide bytes against which to compute the crc.  This method is
     * meant to be only called once between clear() and get().
     *
     * @param[in] bytes - the data against which to compute.
     * @param[in] length - the number of bytes.
     */
    virtual void compute(const uint8_t* bytes, uint32_t length) = 0;

    /**
     * Read back the current crc value.
     *
     * @return the crc16 value.
     */
    virtual uint16_t get() const = 0;
};

class Crc16 : public CrcInterface
{
  public:
    Crc16() : poly(crc16Ccitt), value(crc16Initial){};
    ~Crc16() = default;

    void clear() override;
    void compute(const uint8_t* bytes, uint32_t length) override;
    uint16_t get() const override;

  private:
    uint16_t poly;
    uint16_t value;
};
} // namespace blobs
