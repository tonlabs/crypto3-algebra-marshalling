//---------------------------------------------------------------------------//
// Copyright (c) 2017-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_MARSHALLING_PROCESSING_CURVE_ELEMENT_HPP
#define CRYPTO3_MARSHALLING_PROCESSING_CURVE_ELEMENT_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <iterator>

#include <nil/marshalling/endianness.hpp>

#include <nil/crypto3/algebra/type_traits.hpp>

#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/curves/curve25519.hpp>

#include <nil/crypto3/marshalling/processing/integral.hpp>
#include <nil/crypto3/marshalling/processing/detail/curve_element.hpp>

namespace nil {
    namespace crypto3 {
        namespace marshalling {
            namespace processing {

                template<std::size_t TSize, typename Endianness, typename G1GroupElement, typename TIter>
                typename std::enable_if<std::is_same<typename algebra::curves::bls12_381::g1_type<
                                                         typename G1GroupElement::coordinates,
                                                         algebra::curves::forms::short_weierstrass>::value_type,
                                                     G1GroupElement>::value,
                                        void>::type
                    curve_element_write_data(const G1GroupElement &point, TIter &iter) {

                    using chunk_type = typename TIter::value_type;

                    constexpr static const chunk_type I_bit = 0x40;

                    typename G1GroupElement::group_type::curve_type::template g1_type<
                        typename algebra::curves::coordinates::affine,
                        typename G1GroupElement::form>::value_type point_affine = point.to_affine();
                    chunk_type m_unit = detail::evaluate_m_unit<chunk_type>(point_affine, true);
                    // TODO: check possibilities for TA

                    if (!(I_bit & m_unit)) {

                        // We assume here, that write_data doesn't change the iter
                        write_data<TSize, Endianness>(
                            point_affine.X.data
                                .template convert_to<typename G1GroupElement::field_type::integral_type>(),
                            iter);
                    }
                    (*iter) |= m_unit;
                }

                template<std::size_t TSize, typename Endianness, typename G2GroupElement, typename TIter>
                typename std::enable_if<std::is_same<typename algebra::curves::bls12_381::g2_type<
                                                         typename G2GroupElement::coordinates,
                                                         algebra::curves::forms::short_weierstrass>::value_type,
                                                     G2GroupElement>::value,
                                        void>::type
                    curve_element_write_data(const G2GroupElement &point, TIter &iter) {

                    using chunk_type = typename TIter::value_type;

                    constexpr static const std::size_t sizeof_field_element =
                        TSize / (G2GroupElement::field_type::arity);
                    constexpr static const std::size_t units_bits = 8;
                    constexpr static const std::size_t chunk_bits = sizeof(typename TIter::value_type) * units_bits;
                    constexpr static const std::size_t sizeof_field_element_chunks_count =
                        (sizeof_field_element / chunk_bits) + ((sizeof_field_element % chunk_bits) ? 1 : 0);

                    constexpr static const chunk_type I_bit = 0x40;

                    typename G2GroupElement::group_type::curve_type::template g2_type<
                        typename algebra::curves::coordinates::affine,
                        typename G2GroupElement::form>::value_type point_affine = point.to_affine();
                    chunk_type m_unit = detail::evaluate_m_unit<chunk_type>(point_affine, true);
                    // TODO: check possibilities for TA

                    if (!(I_bit & m_unit)) {

                        TIter write_iter = iter;
                        // We assume here, that write_data doesn't change the iter
                        write_data<sizeof_field_element, Endianness>(
                            point_affine.X.data[1]
                                .data.template convert_to<typename G2GroupElement::field_type::integral_type>(),
                            write_iter);

                        write_iter += sizeof_field_element_chunks_count;
                        // We assume here, that write_data doesn't change the iter
                        write_data<sizeof_field_element, Endianness>(
                            point_affine.X.data[0]
                                .data.template convert_to<typename G2GroupElement::field_type::integral_type>(),
                            write_iter);
                    }
                    (*iter) |= m_unit;
                }

                template<std::size_t TSize, typename Endianness, typename G1GroupElement, typename TIter>
                typename std::enable_if<

                    std::is_same<typename algebra::curves::curve25519::g1_type<
                                     typename G1GroupElement::coordinates,
                                     algebra::curves::forms::twisted_edwards>::value_type,
                                 G1GroupElement>::value &&
                        std::is_same<std::uint8_t, typename std::iterator_traits<TIter>::value_type>::value &&
                        std::is_same<nil::marshalling::endian::little_endian, Endianness>::value,
                    void>::type
                    curve_element_write_data(const G1GroupElement &point, TIter &iter) {
                    using group_value_type = G1GroupElement;
                    using group_type = typename group_value_type::group_type;
                    using base_field_type = typename group_type::field_type;
                    using base_integral_type = typename base_field_type::integral_type;
                    using group_affine_value_type =
                        typename algebra::curves::curve25519::g1_type<algebra::curves::coordinates::affine,
                                                                      typename G1GroupElement::form>::value_type;
                    // TODO: somehow add size check of container pointed by iter
                    constexpr std::size_t encoded_size = 32;
                    static_assert(encoded_size == (TSize / 8 + (TSize % 8 ? 1 : 0)), "wrong size");
                    using encoded_value_type = std::array<std::uint8_t, encoded_size>;

                    group_affine_value_type point_affine = point.to_affine();
                    // TODO: remove crating of temporary array encoded_value
                    encoded_value_type encoded_value;
                    // TODO: remove lvalue iterator
                    auto tmp_iter = std::begin(encoded_value);
                    write_data<encoded_size, Endianness>(static_cast<base_integral_type>(point_affine.Y.data),
                                                         tmp_iter);
                    // TODO: throw catchable error, for example return status
                    assert(!(encoded_value[encoded_size - 1] & 0x80));
                    encoded_value[encoded_size - 1] |=
                        (static_cast<std::uint8_t>(static_cast<base_integral_type>(point_affine.X.data) & 1) << 7);

                    std::copy(std::cbegin(encoded_value), std::cend(encoded_value), iter);
                }

                template<std::size_t TSize, typename Endianness, typename G1GroupElement, typename TIter>
                typename std::enable_if<std::is_same<typename algebra::curves::bls12_381::g1_type<
                                                         typename G1GroupElement::coordinates,
                                                         algebra::curves::forms::short_weierstrass>::value_type,
                                                     G1GroupElement>::value,
                                        G1GroupElement>::type
                    curve_element_read_data(TIter &iter) {

                    using chunk_type = typename TIter::value_type;

                    const chunk_type m_unit = *iter & 0xE0;
                    BOOST_ASSERT(m_unit != 0x20 && m_unit != 0x60 && m_unit != 0xE0);

                    constexpr static const std::size_t sizeof_field_element =
                        TSize / (G1GroupElement::field_type::arity);
                    constexpr static const std::size_t units_bits = 8;
                    constexpr static const std::size_t chunk_bits = sizeof(chunk_type) * units_bits;
                    constexpr static const std::size_t sizeof_field_element_chunks_count =
                        (sizeof_field_element / chunk_bits) + ((sizeof_field_element % chunk_bits) ? 1 : 0);
                    using g1_value_type = G1GroupElement;
                    using g1_field_type = typename g1_value_type::field_type;
                    using g1_field_value_type = typename g1_field_type::value_type;

                    constexpr static const chunk_type I_bit = 0x40;
                    constexpr static const chunk_type S_bit = 0x20;

                    if (m_unit & I_bit) {
                        BOOST_ASSERT(iter + sizeof_field_element_chunks_count ==
                                     std::find(iter, iter + sizeof_field_element_chunks_count, true));
                        return g1_value_type();    // point at infinity
                    }

                    typename G1GroupElement::field_type::integral_type x =
                        read_data<sizeof_field_element, typename G1GroupElement::field_type::integral_type, Endianness>(
                            iter);

                    g1_field_value_type x_mod(x);
                    g1_field_value_type y2_mod = x_mod.pow(3) + g1_field_value_type(4);
                    BOOST_ASSERT(y2_mod.is_square());
                    g1_field_value_type y_mod = y2_mod.sqrt();
                    bool Y_bit = detail::sign_gf_p<g1_field_type>(y_mod);
                    if (Y_bit == bool(m_unit & S_bit)) {
                        g1_value_type result(x_mod, y_mod, g1_field_value_type::one());
                        BOOST_ASSERT(result.is_well_formed());
                        return result;
                    }
                    g1_value_type result(x_mod, -y_mod, g1_field_value_type::one());
                    BOOST_ASSERT(result.is_well_formed());
                    return result;
                }

                template<std::size_t TSize, typename Endianness, typename G2GroupElement, typename TIter>
                typename std::enable_if<std::is_same<typename algebra::curves::bls12_381::g2_type<
                                                         typename G2GroupElement::coordinates,
                                                         algebra::curves::forms::short_weierstrass>::value_type,
                                                     G2GroupElement>::value,
                                        G2GroupElement>::type
                    curve_element_read_data(TIter &iter) {

                    using chunk_type = typename TIter::value_type;

                    const chunk_type m_unit = *iter & 0xE0;
                    BOOST_ASSERT(m_unit != 0x20 && m_unit != 0x60 && m_unit != 0xE0);

                    constexpr static const std::size_t sizeof_field_element =
                        TSize / (G2GroupElement::field_type::arity);
                    constexpr static const std::size_t units_bits = 8;
                    constexpr static const std::size_t chunk_bits = sizeof(chunk_type) * units_bits;
                    constexpr static const std::size_t sizeof_field_element_chunks_count =
                        (sizeof_field_element / chunk_bits) + ((sizeof_field_element % chunk_bits) ? 1 : 0);
                    using g2_value_type = G2GroupElement;
                    using g2_field_type = typename g2_value_type::field_type;
                    using g2_field_value_type = typename g2_field_type::value_type;

                    constexpr static const chunk_type I_bit = 0x40;
                    constexpr static const chunk_type S_bit = 0x20;

                    if (m_unit & I_bit) {
                        BOOST_ASSERT(iter + 2 * sizeof_field_element_chunks_count ==
                                     std::find(iter, iter + 2 * sizeof_field_element_chunks_count, true));
                        return g2_value_type();    // point at infinity
                    }

                    TIter read_iter = iter;

                    typename G2GroupElement::field_type::integral_type x_1 =
                        read_data<sizeof_field_element, typename G2GroupElement::field_type::integral_type, Endianness>(
                            read_iter);
                    read_iter += sizeof_field_element_chunks_count;

                    typename G2GroupElement::field_type::integral_type x_0 =
                        read_data<sizeof_field_element, typename G2GroupElement::field_type::integral_type, Endianness>(
                            read_iter);

                    g2_field_value_type x_mod(x_0, x_1);
                    g2_field_value_type y2_mod = x_mod.pow(3) + g2_field_value_type(4, 4);
                    BOOST_ASSERT(y2_mod.is_square());
                    g2_field_value_type y_mod = y2_mod.sqrt();
                    bool Y_bit = detail::sign_gf_p<g2_field_type>(y_mod);
                    if (Y_bit == bool(m_unit & S_bit)) {
                        g2_value_type result(x_mod, y_mod, g2_field_value_type::one());
                        BOOST_ASSERT(result.is_well_formed());
                        return result;
                    }
                    g2_value_type result(x_mod, -y_mod, g2_field_value_type::one());
                    BOOST_ASSERT(result.is_well_formed());
                    return result;
                }

                template<std::size_t TSize, typename Endianness, typename G1GroupElement, typename TIter>
                typename std::enable_if<
                    std::is_same<typename algebra::curves::curve25519::g1_type<
                                     typename G1GroupElement::coordinates,
                                     algebra::curves::forms::twisted_edwards>::value_type,
                                 G1GroupElement>::value &&
                        std::is_same<std::uint8_t, typename std::iterator_traits<TIter>::value_type>::value &&
                        std::is_same<nil::marshalling::endian::little_endian, Endianness>::value,
                    G1GroupElement>::type
                    curve_element_read_data(TIter &iter) {
                    // somehow add size check of container pointed by iter
                    // assert(TSize == std::distance(first, last));
                    using group_value_type = G1GroupElement;
                    using group_type = typename group_value_type::group_type;
                    using base_field_type = typename group_type::field_type;
                    using base_integral_type = typename base_field_type::integral_type;
                    using group_affine_value_type =
                        typename algebra::curves::curve25519::g1_type<algebra::curves::coordinates::affine,
                                                                      typename G1GroupElement::form>::value_type;
                    constexpr std::size_t encoded_size = 32;
                    static_assert(encoded_size == (TSize / 8 + (TSize % 8 ? 1 : 0)), "wrong size");

                    base_integral_type y = read_data<TSize, base_integral_type, Endianness>(iter);
                    bool sign = *(iter + encoded_size - 1) & (1 << 7);
                    group_affine_value_type decoded_point_affine = detail::recover_x<group_affine_value_type>(y, sign);

                    // TODO: remove hard-coded call for type conversion, implement type conversion between coordinates
                    //  through operator
                    return decoded_point_affine.to_extended_with_a_minus_1();
                }
            }    // namespace processing
        }        // namespace marshalling
    }            // namespace crypto3
}    // namespace nil
#endif    // CRYPTO3_MARSHALLING_PROCESSING_CURVE_ELEMENT_HPP
