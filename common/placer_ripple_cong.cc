/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2019-2020  David Shah <dave@ds0.me>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  [[cite]] RippleFPGA
 *  RippleFPGA: A Routability-Driven Placement forLarge-Scale Heterogeneous FPGAs
 *  Chak-Wa Pui, Gengjie Chen, Wing-Kai Chow, Ka-Chun Lam, Jian Kuang,Peishan Tu, Hang Zhang, Evangeline F. Y. Young,
 * Bei Yu https://chengengjie.github.io/papers/C2-ICCAD16-RippleFPGA.pdf
 *
 *
 *  Original Implementation: https://github.com/cuhk-eda/ripple-fpga
 *
 *  Original Copyright:
 *
 * Copyright (c) 2019 by The Chinese University of Hong Kong
 * All rights reserved
 * CU-SD LICENSE (adapted from the original BSD license)
 * Redistribution of the any code, with or without modification, are permitted provided that the conditions below are
 * met. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name nor
 * trademark of the copyright holder or the author may be used to endorse or promote products derived from this software
 * without specific prior written permission. Users are entirely responsible, to the exclusion of the author, for
 * compliance with (a) regulations set by owners or administrators of employed equipment, (b) licensing terms of any
 * other software, and (c) local, national, and international regulations regarding use, including those regarding
 * import, export, and use of encryption software. THIS FREE SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR ANY CONTRIBUTOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, EFFECTS OF
 * UNAUTHORIZED OR MALICIOUS NETWORK ACCESS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include "nextpnr.h"
#include "place_common.h"
#include "placer1.h"
#include "placer_ripple_int.h"
#include "util.h"

NEXTPNR_NAMESPACE_BEGIN
namespace Ripple {
void RippleFPGAPlacer::est_congestion_map(array2d<double> &cong)
{
    for (int x = 0; x < d.width; x++) {
        for (int y = 0; y < d.height; y++) {
            cong.at(x, y) = 0;
        }
    }

    auto get_pincount_weight = [](int pins) {
        // Magic numbers from
        // https://github.com/cuhk-eda/ripple-fpga/blob/059646fcaa0c8d77e4283f55be2bd5692862a039/src/cong/cong_est_bb.cpp#L21-L34
        if (pins < 10)
            return 1.06;
        else if (pins < 20)
            return 1.2;
        else if (pins < 30)
            return 1.4;
        else if (pins < 50)
            return 1.6;
        else if (pins < 100)
            return 1.8;
        else if (pins < 200)
            return 2.1;
        else
            return 3.0;
    };

    for (auto net : sorted(ctx->nets)) {
        if (packed_nets.count(net.first))
            continue;
        NetInfo *ni = net.second;
        if (ni->driver.cell == nullptr)
            continue;
        Bounds bb;
        bb.expand(f->getSwitchbox(get_cell_location(ni->driver.cell)));
        for (const auto &usr : ni->users)
            bb.expand(f->getSwitchbox(get_cell_location(usr.cell)));
        double weight = (bb.hpwl() + 1) * get_pincount_weight(GetSize(ni->users) + 1);
        int num_switchboxes = ((bb.x1 - bb.x0) + 1) * ((bb.y1 - bb.y0) + 1);
        weight /= num_switchboxes;
        for (int x = bb.x0; x <= bb.x1; x++) {
            for (int y = bb.y0; y <= bb.y1; y++) {
                cong.at(x, y) += weight;
            }
        }
    }
}
} // namespace Ripple
NEXTPNR_NAMESPACE_END