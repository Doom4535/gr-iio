/* -*- c++ -*- */
/*
 * Copyright 2019 Travis Collins <travis.collins@analog.com>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "attr_updater_impl.h"

namespace gr {
  namespace iio {

    attr_updater::sptr
    attr_updater::make(const std::string attribute, const std::string value, unsigned int interval_ms)
    {
      return gnuradio::get_initial_sptr
        (new attr_updater_impl(attribute, value, interval_ms));
    }

    /*
     * The private constructor
     */
    attr_updater_impl::attr_updater_impl(const std::string attribute, const std::string value,
      unsigned int interval_ms)
      : gr::block("attr_updater",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0)),
        d_attribute(attribute),
        d_value(value),
        d_port(pmt::mp("out")),
        d_interval_ms(interval_ms),
        d_finished(false)
    {
      d_msg = pmt::make_dict();
      pmt::pmt_t key = pmt::string_to_symbol(d_attribute);
      pmt::pmt_t dvalue = pmt::string_to_symbol(value);
      d_msg = pmt::dict_add(d_msg, key, dvalue);
      message_port_register_out(d_port);
    }

    /*
     * Our virtual destructor.
     */
    attr_updater_impl::~attr_updater_impl()
    {
    }

    void attr_updater_impl::set_value(std::string value)
    {
      // Check if message is different
      if (d_value.compare(value)==0)
        return;
      else
        d_value = value;

      // Build message
      pmt::pmt_t msg = pmt::make_dict();
      pmt::pmt_t key = pmt::string_to_symbol(d_attribute);
      pmt::pmt_t dvalue = pmt::string_to_symbol(value);
      msg = pmt::dict_add(msg, key, dvalue);
      // Update
      if (d_interval_ms==0)
        message_port_pub(d_port, msg);
      else {
        d_mtx.lock();
        d_msg = msg;
        d_mtx.unlock();
      }
    }

    bool attr_updater_impl::start()
    {
        if (d_interval_ms<=0.0)
        {
          message_port_pub(d_port, d_msg);
        }
        else {
          d_mtx.lock();
          d_finished = false;
          d_mtx.unlock();
          d_thread = boost::shared_ptr<gr::thread::thread>(
              new gr::thread::thread(boost::bind(&attr_updater_impl::run, this)));
        }
        return block::start();
    }

    bool attr_updater_impl::stop()
    {
        // Shut down the thread
        d_finished = true;
        d_thread->interrupt();
        d_thread->join();

        return block::stop();
    }


    void attr_updater_impl::run()
    {
        while (!d_finished) {
            boost::this_thread::sleep(
                boost::posix_time::milliseconds(static_cast<long>(d_interval_ms)));
            if (d_finished) {
                return;
            }
            d_mtx.lock();
            message_port_pub(d_port, d_msg);
            d_mtx.unlock();
        }
    }



  } /* namespace iio */
} /* namespace gr */
