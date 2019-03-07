/******************************************
Copyright (c) 2018, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "clause.h"
#include "reducedb.h"

namespace CMSat {

static double estimator_should_keep_short_conf4_cluster0_0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_last_touched_diff <= 24091.0f ) {
        if ( cl->stats.antec_num_total_lits_rel <= 0.482848644257f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.350552737713f ) {
                    if ( rdb0_last_touched_diff <= 9690.5f ) {
                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.000982520869002f ) {
                                return 35.0/2172.4;
                            } else {
                                if ( rdb0_last_touched_diff <= 3519.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0689656883478f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                return 392.0/2970.3;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                    return 157.0/1764.3;
                                                } else {
                                                    return 130.0/2692.2;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0589611679316f ) {
                                                    return 160.0/2001.9;
                                                } else {
                                                    return 126.0/2470.9;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1526.0f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.210712492466f ) {
                                                        if ( cl->stats.glue_rel_long <= 0.153676480055f ) {
                                                            return 82.0/1924.7;
                                                        } else {
                                                            return 62.0/2095.3;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                            if ( cl->stats.rdb1_used_for_uip_creation <= 18.5f ) {
                                                                return 59.0/2962.2;
                                                            } else {
                                                                return 30.0/3666.7;
                                                            }
                                                        } else {
                                                            return 90.0/3928.6;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 1055.0f ) {
                                                        return 57.0/2308.4;
                                                    } else {
                                                        return 83.0/1997.8;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1051.5f ) {
                                            if ( rdb0_last_touched_diff <= 178.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                    return 80.0/2543.9;
                                                } else {
                                                    return 30.0/3975.3;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                    return 179.0/2604.9;
                                                } else {
                                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                        return 37.0/3325.6;
                                                    } else {
                                                        if ( cl->stats.dump_number <= 8.5f ) {
                                                            return 53.0/2105.4;
                                                        } else {
                                                            if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                                                return 52.0/2099.3;
                                                            } else {
                                                                return 23.0/2166.3;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0556275993586f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                                    return 311.0/4080.9;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                        return 74.0/1971.4;
                                                    } else {
                                                        return 57.0/2141.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 4755.0f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                                        return 99.0/1943.0;
                                                    } else {
                                                        return 82.0/3289.1;
                                                    }
                                                } else {
                                                    return 123.0/1880.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 14896.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.717143118382f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0259487107396f ) {
                                                return 212.0/1638.4;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                    if ( cl->size() <= 6.5f ) {
                                                        if ( cl->stats.size_rel <= 0.206270694733f ) {
                                                            return 329.0/2718.6;
                                                        } else {
                                                            return 187.0/1953.1;
                                                        }
                                                    } else {
                                                        return 374.0/2749.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 0.323958873749f ) {
                                                            return 202.0/1713.6;
                                                        } else {
                                                            if ( cl->stats.glue_rel_long <= 0.460029959679f ) {
                                                                return 126.0/1953.1;
                                                            } else {
                                                                return 150.0/1809.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 25.5f ) {
                                                            if ( cl->size() <= 6.5f ) {
                                                                return 155.0/3327.6;
                                                            } else {
                                                                return 127.0/2064.8;
                                                            }
                                                        } else {
                                                            return 77.0/2099.3;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 300.0/2633.3;
                                        }
                                    } else {
                                        return 326.0/1756.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.354654312134f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 7124.5f ) {
                                    if ( rdb0_last_touched_diff <= 5327.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0567649081349f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                                return 164.0/2700.3;
                                            } else {
                                                return 71.0/1959.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 618.0f ) {
                                                return 96.0/3644.4;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 827.5f ) {
                                                    return 53.0/2373.4;
                                                } else {
                                                    return 183.0/3315.5;
                                                }
                                            }
                                        }
                                    } else {
                                        return 243.0/2312.5;
                                    }
                                } else {
                                    return 280.0/2430.3;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( rdb0_last_touched_diff <= 4543.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.566911280155f ) {
                                            return 385.0/2785.5;
                                        } else {
                                            return 370.0/3297.2;
                                        }
                                    } else {
                                        if ( cl->size() <= 10.5f ) {
                                            return 382.0/2633.3;
                                        } else {
                                            return 462.0/2326.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.202892914414f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0829361975193f ) {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 119.0/2257.7;
                                                } else {
                                                    return 235.0/3429.1;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0821101516485f ) {
                                                    return 130.0/3467.7;
                                                } else {
                                                    return 198.0/3114.5;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                return 227.0/3134.8;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 1536.0f ) {
                                                    return 60.0/3774.3;
                                                } else {
                                                    return 102.0/2548.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                            return 146.0/2409.9;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 732.5f ) {
                                                return 31.0/2304.4;
                                            } else {
                                                return 66.0/2162.3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( rdb0_last_touched_diff <= 18339.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.417117297649f ) {
                                            return 380.0/1524.7;
                                        } else {
                                            return 378.0/1333.9;
                                        }
                                    } else {
                                        return 382.0/1807.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6035.5f ) {
                                        return 389.0/1327.8;
                                    } else {
                                        return 374.0/1319.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                    if ( cl->stats.size_rel <= 0.182658404112f ) {
                                        return 298.0/1778.5;
                                    } else {
                                        return 218.0/1652.7;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 17.5f ) {
                                        return 385.0/2615.0;
                                    } else {
                                        return 255.0/1865.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                    return 543.0/2206.9;
                                } else {
                                    if ( cl->stats.size_rel <= 0.152797818184f ) {
                                        return 286.0/1668.9;
                                    } else {
                                        return 384.0/1725.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 1.5f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        return 497.0/1185.7;
                                    } else {
                                        return 790.0/1238.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        if ( rdb0_last_touched_diff <= 11864.0f ) {
                                            return 501.0/1754.2;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.548102378845f ) {
                                                if ( cl->stats.glue_rel_long <= 0.563821792603f ) {
                                                    if ( cl->stats.dump_number <= 13.5f ) {
                                                        return 411.0/1273.0;
                                                    } else {
                                                        return 596.0/1224.3;
                                                    }
                                                } else {
                                                    return 653.0/2062.8;
                                                }
                                            } else {
                                                return 857.0/1815.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.690250754356f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                                return 431.0/2342.9;
                                            } else {
                                                return 209.0/1841.5;
                                            }
                                        } else {
                                            return 319.0/1652.7;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.842185854912f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4797.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        if ( rdb0_last_touched_diff <= 11090.0f ) {
                                            return 356.0/1514.6;
                                        } else {
                                            return 540.0/1285.2;
                                        }
                                    } else {
                                        return 407.0/3280.9;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.266548931599f ) {
                                        return 323.0/1536.9;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 10916.5f ) {
                                                return 530.0/1070.0;
                                            } else {
                                                return 493.0/1624.2;
                                            }
                                        } else {
                                            return 443.0/1618.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.532540023327f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3555.5f ) {
                                        return 385.0/1415.1;
                                    } else {
                                        return 413.0/1197.9;
                                    }
                                } else {
                                    return 638.0/1299.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.534162282944f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.160547360778f ) {
                                        if ( cl->stats.glue_rel_long <= 0.40478682518f ) {
                                            return 229.0/2105.4;
                                        } else {
                                            return 168.0/2014.0;
                                        }
                                    } else {
                                        return 246.0/3429.1;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                        return 65.0/2466.8;
                                    } else {
                                        return 170.0/3605.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 123.0/2026.2;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.606200039387f ) {
                                        return 326.0/3453.5;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 269.0/1833.3;
                                        } else {
                                            if ( cl->stats.dump_number <= 14.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 2301.0f ) {
                                                    return 116.0/2231.3;
                                                } else {
                                                    return 159.0/1790.7;
                                                }
                                            } else {
                                                return 102.0/2060.7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( rdb0_last_touched_diff <= 8669.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 418.0/1707.5;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 232.0/1882.1;
                                    } else {
                                        return 190.0/3317.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 583.0/1388.7;
                                } else {
                                    return 789.0/1266.9;
                                }
                            }
                        } else {
                            return 1171.0/1687.2;
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.dump_number <= 2.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.634883522987f ) {
                            if ( cl->size() <= 11.5f ) {
                                return 680.0/2263.8;
                            } else {
                                return 610.0/1013.1;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 82.5f ) {
                                if ( rdb0_last_touched_diff <= 18022.5f ) {
                                    return 592.0/931.9;
                                } else {
                                    return 687.0/653.8;
                                }
                            } else {
                                return 977.0/454.8;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.023749999702f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 6889.5f ) {
                                    return 440.0/2211.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 29057.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 410.0/1766.3;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.633123636246f ) {
                                                if ( cl->size() <= 7.5f ) {
                                                    return 478.0/1729.8;
                                                } else {
                                                    return 411.0/1268.9;
                                                }
                                            } else {
                                                return 451.0/1153.2;
                                            }
                                        }
                                    } else {
                                        return 598.0/1528.8;
                                    }
                                }
                            } else {
                                return 628.0/1305.5;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    return 497.0/1551.1;
                                } else {
                                    return 526.0/1094.3;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.264024019241f ) {
                                    return 1064.0/1766.3;
                                } else {
                                    return 582.0/1376.5;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.520293295383f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 18481.0f ) {
                            if ( rdb0_last_touched_diff <= 1614.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.403822869062f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.295235037804f ) {
                                        return 111.0/2546.0;
                                    } else {
                                        return 118.0/1930.8;
                                    }
                                } else {
                                    return 86.0/2075.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.311364889145f ) {
                                    return 226.0/2732.8;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.406099736691f ) {
                                        return 214.0/1760.3;
                                    } else {
                                        return 181.0/1961.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                if ( cl->stats.dump_number <= 16.5f ) {
                                    return 294.0/2028.3;
                                } else {
                                    return 286.0/2529.7;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.163364455104f ) {
                                    return 377.0/1845.5;
                                } else {
                                    return 350.0/2328.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                            if ( cl->size() <= 5.5f ) {
                                return 189.0/1859.7;
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        return 397.0/1587.7;
                                    } else {
                                        return 404.0/2826.2;
                                    }
                                } else {
                                    return 179.0/3151.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 10.5f ) {
                                if ( rdb0_last_touched_diff <= 2969.5f ) {
                                    return 360.0/2742.9;
                                } else {
                                    return 434.0/1561.3;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 831.5f ) {
                                    return 130.0/1979.5;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 388.0/1508.5;
                                    } else {
                                        return 211.0/2028.3;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.glue_rel_long <= 1.04850137234f ) {
                        if ( cl->stats.glue_rel_queue <= 0.804056823254f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 696.0/2198.8;
                            } else {
                                return 435.0/1857.7;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 10320.0f ) {
                                return 437.0/1307.5;
                            } else {
                                return 1113.0/1666.9;
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.772107064724f ) {
                            return 667.0/850.7;
                        } else {
                            return 935.0/619.2;
                        }
                    }
                } else {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.77256202698f ) {
                            return 156.0/3366.2;
                        } else {
                            return 193.0/1869.9;
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                            if ( cl->stats.size_rel <= 0.811326026917f ) {
                                return 407.0/2631.2;
                            } else {
                                return 530.0/2416.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.438550174236f ) {
                                return 64.0/1997.8;
                            } else {
                                if ( cl->stats.glue <= 13.5f ) {
                                    return 181.0/2765.2;
                                } else {
                                    return 96.0/1900.3;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.876795887947f ) {
                    if ( cl->stats.glue_rel_queue <= 0.813756346703f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.586088180542f ) {
                            return 332.0/1411.0;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 158.5f ) {
                                return 498.0/1092.3;
                            } else {
                                return 593.0/1039.5;
                            }
                        }
                    } else {
                        return 800.0/1035.4;
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 9943.0f ) {
                        return 426.0/1240.5;
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 8.72486114502f ) {
                                if ( cl->stats.glue_rel_long <= 1.09728145599f ) {
                                    return 758.0/548.2;
                                } else {
                                    return 983.0/330.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.17084562778f ) {
                                    return 1407.0/330.9;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 364.5f ) {
                                        return 1041.0/123.8;
                                    } else {
                                        return 968.0/77.2;
                                    }
                                }
                            }
                        } else {
                            return 758.0/1155.2;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue_rel_long <= 0.779468119144f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.size_rel <= 0.483565270901f ) {
                    if ( rdb0_last_touched_diff <= 54205.0f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.316514611244f ) {
                                    return 589.0/1453.7;
                                } else {
                                    if ( rdb0_last_touched_diff <= 30809.5f ) {
                                        return 383.0/1646.6;
                                    } else {
                                        return 573.0/2038.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0307492818683f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0446155592799f ) {
                                        return 813.0/1670.9;
                                    } else {
                                        return 569.0/1005.0;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 723.0/1752.1;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.298362791538f ) {
                                            return 466.0/1092.3;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 44711.0f ) {
                                                if ( cl->stats.dump_number <= 12.5f ) {
                                                    return 368.0/1380.6;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.122754812241f ) {
                                                        return 400.0/1327.8;
                                                    } else {
                                                        return 449.0/1289.2;
                                                    }
                                                }
                                            } else {
                                                return 487.0/1061.8;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 34039.0f ) {
                                if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.496008306742f ) {
                                        return 475.0/1218.2;
                                    } else {
                                        return 535.0/1045.6;
                                    }
                                } else {
                                    return 629.0/1021.2;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.12294241786f ) {
                                    return 968.0/1557.2;
                                } else {
                                    return 1111.0/1409.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.size_rel <= 0.235204994678f ) {
                                return 525.0/1153.2;
                            } else {
                                return 589.0/921.7;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 137473.5f ) {
                                if ( cl->size() <= 12.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.501840114594f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( cl->stats.size_rel <= 0.143512159586f ) {
                                                return 558.0/1070.0;
                                            } else {
                                                return 612.0/1000.9;
                                            }
                                        } else {
                                            return 1182.0/1530.8;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 23.5f ) {
                                            return 1115.0/1338.0;
                                        } else {
                                            return 596.0/907.5;
                                        }
                                    }
                                } else {
                                    return 1346.0/881.1;
                                }
                            } else {
                                return 849.0/588.8;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.829556405544f ) {
                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0533380880952f ) {
                                return 856.0/1080.1;
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    return 690.0/1271.0;
                                } else {
                                    return 745.0/1061.8;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 43465.0f ) {
                                if ( rdb0_last_touched_diff <= 41641.5f ) {
                                    if ( cl->size() <= 16.5f ) {
                                        return 839.0/1522.7;
                                    } else {
                                        return 964.0/1283.1;
                                    }
                                } else {
                                    return 810.0/948.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    return 766.0/661.9;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.37966060638f ) {
                                        return 870.0/519.8;
                                    } else {
                                        return 816.0/625.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.698096752167f ) {
                            if ( cl->size() <= 27.5f ) {
                                return 868.0/661.9;
                            } else {
                                return 1363.0/1250.7;
                            }
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 806.0/501.5;
                            } else {
                                return 944.0/621.3;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 121061.0f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0992369651794f ) {
                        if ( cl->stats.glue_rel_long <= 0.418566524982f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                return 690.0/1532.9;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                    return 1001.0/1256.7;
                                } else {
                                    if ( cl->stats.size_rel <= 0.208043754101f ) {
                                        return 575.0/1061.8;
                                    } else {
                                        return 628.0/828.4;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.48118236661f ) {
                                if ( rdb0_last_touched_diff <= 91039.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.533254384995f ) {
                                        return 814.0/970.5;
                                    } else {
                                        return 989.0/1437.4;
                                    }
                                } else {
                                    return 907.0/858.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 62400.0f ) {
                                    return 678.0/877.1;
                                } else {
                                    return 881.0/580.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.598147630692f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 53.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 778.0/1139.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.146765530109f ) {
                                        return 763.0/572.5;
                                    } else {
                                        return 801.0/864.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 45340.5f ) {
                                    return 773.0/663.9;
                                } else {
                                    return 1534.0/836.5;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.659056186676f ) {
                                if ( cl->stats.size_rel <= 0.851953625679f ) {
                                    return 818.0/538.0;
                                } else {
                                    return 865.0/383.7;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                    return 835.0/402.0;
                                } else {
                                    return 1214.0/286.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 8.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 271644.0f ) {
                            if ( cl->stats.glue_rel_long <= 0.528808772564f ) {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 916.0/1163.4;
                                } else {
                                    if ( cl->stats.dump_number <= 30.5f ) {
                                        return 853.0/609.1;
                                    } else {
                                        return 700.0/730.9;
                                    }
                                }
                            } else {
                                return 1031.0/720.8;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 63.5f ) {
                                return 1194.0/586.8;
                            } else {
                                return 835.0/674.1;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 277577.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.492055475712f ) {
                                if ( cl->stats.size_rel <= 0.505822777748f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 826.0/655.8;
                                    } else {
                                        return 942.0/513.7;
                                    }
                                } else {
                                    return 1023.0/395.9;
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 983.0/590.8;
                                } else {
                                    if ( cl->stats.size_rel <= 0.947592496872f ) {
                                        if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                            if ( cl->size() <= 20.5f ) {
                                                if ( cl->stats.size_rel <= 0.615221619606f ) {
                                                    return 1398.0/676.1;
                                                } else {
                                                    return 1087.0/326.9;
                                                }
                                            } else {
                                                return 979.0/444.6;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.644805908203f ) {
                                                return 845.0/326.9;
                                            } else {
                                                return 1076.0/333.0;
                                            }
                                        }
                                    } else {
                                        return 1782.0/436.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 97.5f ) {
                                if ( cl->size() <= 20.5f ) {
                                    if ( rdb0_last_touched_diff <= 335452.0f ) {
                                        return 1030.0/357.3;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 1317.0/373.6;
                                        } else {
                                            return 1279.0/278.1;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        return 1390.0/272.1;
                                    } else {
                                        return 1139.0/174.6;
                                    }
                                }
                            } else {
                                return 929.0/422.3;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_total_lits_antecedents <= 56.5f ) {
                if ( cl->stats.size_rel <= 0.737866401672f ) {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 4.0612244606f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 50804.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 28827.0f ) {
                                    return 994.0/2036.4;
                                } else {
                                    return 714.0/1106.5;
                                }
                            } else {
                                return 1012.0/844.6;
                            }
                        } else {
                            return 747.0/680.1;
                        }
                    } else {
                        if ( cl->stats.glue <= 6.5f ) {
                            return 1229.0/1246.6;
                        } else {
                            if ( cl->stats.dump_number <= 17.5f ) {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 1553.0/1065.9;
                                } else {
                                    return 1395.0/645.6;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                    return 1629.0/757.3;
                                } else {
                                    return 1148.0/359.4;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 67653.0f ) {
                        if ( cl->stats.dump_number <= 7.5f ) {
                            if ( rdb0_last_touched_diff <= 39406.5f ) {
                                return 721.0/617.2;
                            } else {
                                return 1353.0/578.6;
                            }
                        } else {
                            return 669.0/802.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.110491916537f ) {
                            return 1341.0/418.2;
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.235840454698f ) {
                                    return 1117.0/213.2;
                                } else {
                                    return 1109.0/302.5;
                                }
                            } else {
                                return 1580.0/237.5;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 45524.0f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.955079913139f ) {
                            if ( cl->stats.glue <= 12.5f ) {
                                return 774.0/864.9;
                            } else {
                                return 1411.0/1114.6;
                            }
                        } else {
                            return 1408.0/710.6;
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                            if ( cl->stats.glue_rel_queue <= 1.04356074333f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 25212.0f ) {
                                    return 869.0/594.9;
                                } else {
                                    return 891.0/554.3;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 210.5f ) {
                                    return 1122.0/548.2;
                                } else {
                                    return 1192.0/233.5;
                                }
                            }
                        } else {
                            if ( cl->size() <= 14.5f ) {
                                return 1008.0/655.8;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0022830569651f ) {
                                    return 917.0/714.7;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.0448846817f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 185.5f ) {
                                            return 1532.0/643.6;
                                        } else {
                                            return 1382.0/373.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 336.5f ) {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                if ( cl->size() <= 27.5f ) {
                                                    return 977.0/215.2;
                                                } else {
                                                    return 1682.0/229.4;
                                                }
                                            } else {
                                                return 978.0/397.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 3.5f ) {
                                                return 1491.0/125.9;
                                            } else {
                                                return 1577.0/229.4;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.252218931913f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 135857.5f ) {
                            if ( cl->stats.dump_number <= 12.5f ) {
                                return 1711.0/605.0;
                            } else {
                                return 831.0/663.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 96.5f ) {
                                return 1304.0/263.9;
                            } else {
                                return 1582.0/428.4;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( cl->stats.glue_rel_queue <= 1.10111951828f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    return 958.0/499.4;
                                } else {
                                    if ( cl->size() <= 20.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 109557.5f ) {
                                            return 1520.0/566.4;
                                        } else {
                                            if ( cl->size() <= 14.5f ) {
                                                return 1221.0/351.2;
                                            } else {
                                                return 1242.0/253.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 157308.0f ) {
                                            return 1804.0/460.9;
                                        } else {
                                            return 1683.0/237.5;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( rdb0_last_touched_diff <= 101738.0f ) {
                                        return 873.0/333.0;
                                    } else {
                                        return 1113.0/213.2;
                                    }
                                } else {
                                    return 1005.0/117.8;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 21.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.874560594559f ) {
                                        if ( cl->size() <= 33.5f ) {
                                            return 907.0/345.1;
                                        } else {
                                            return 891.0/270.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 56.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.07943546772f ) {
                                                    return 891.0/266.0;
                                                } else {
                                                    return 935.0/152.3;
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 33.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.883475720882f ) {
                                                        if ( rdb0_last_touched_diff <= 107657.5f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 117.5f ) {
                                                                return 1220.0/253.8;
                                                            } else {
                                                                return 1274.0/207.1;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.67011988163f ) {
                                                                if ( cl->size() <= 32.5f ) {
                                                                    return 1249.0/85.3;
                                                                } else {
                                                                    return 1182.0/168.5;
                                                                }
                                                            } else {
                                                                return 1377.0/243.6;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 1.33651959896f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 93869.5f ) {
                                                                return 1547.0/219.3;
                                                            } else {
                                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                                    return 1079.0/62.9;
                                                                } else {
                                                                    return 1561.0/209.1;
                                                                }
                                                            }
                                                        } else {
                                                            return 1474.0/103.5;
                                                        }
                                                    }
                                                } else {
                                                    return 1700.0/322.8;
                                                }
                                            }
                                        } else {
                                            return 994.0/312.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.991177558899f ) {
                                        return 980.0/107.6;
                                    } else {
                                        if ( cl->stats.dump_number <= 24.5f ) {
                                            if ( rdb0_last_touched_diff <= 148758.5f ) {
                                                return 1125.0/62.9;
                                            } else {
                                                return 1231.0/113.7;
                                            }
                                        } else {
                                            return 1362.0/52.8;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.512164235115f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 984.0/201.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.884142637253f ) {
                                            return 1492.0/270.0;
                                        } else {
                                            return 1259.0/123.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.01688063145f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            if ( cl->stats.glue <= 35.5f ) {
                                                return 1164.0/87.3;
                                            } else {
                                                return 1077.0/97.5;
                                            }
                                        } else {
                                            return 1701.0/263.9;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 95324.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                return 1414.0/101.5;
                                            } else {
                                                return 1301.0/182.7;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.40116477013f ) {
                                                if ( cl->stats.glue <= 49.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 54.8645629883f ) {
                                                        return 1417.0/91.4;
                                                    } else {
                                                        return 978.0/97.5;
                                                    }
                                                } else {
                                                    return 1695.0/65.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                    return 1804.0/85.3;
                                                } else {
                                                    return 1091.0/10.2;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_1(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_last_touched_diff <= 24263.5f ) {
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.size_rel <= 0.532244920731f ) {
                if ( cl->stats.glue_rel_queue <= 0.520293295383f ) {
                    if ( cl->size() <= 5.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0336970686913f ) {
                            if ( rdb0_last_touched_diff <= 3529.5f ) {
                                if ( rdb0_last_touched_diff <= 521.5f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->stats.size_rel <= 0.0713449716568f ) {
                                            return 34.0/2414.0;
                                        } else {
                                            return 72.0/2005.9;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 17.5f ) {
                                            return 118.0/1926.7;
                                        } else {
                                            return 58.0/2426.2;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 9896.0f ) {
                                            return 296.0/3398.7;
                                        } else {
                                            return 251.0/2040.4;
                                        }
                                    } else {
                                        if ( cl->size() <= 3.5f ) {
                                            return 48.0/2436.3;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1499.0f ) {
                                                return 81.0/2426.2;
                                            } else {
                                                return 103.0/1902.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 4793.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2006.0f ) {
                                        return 339.0/3390.6;
                                    } else {
                                        return 271.0/2044.5;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 17.5f ) {
                                        return 300.0/1673.0;
                                    } else {
                                        return 422.0/1908.5;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3928.0f ) {
                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.40243190527f ) {
                                        if ( rdb0_last_touched_diff <= 2119.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.26053583622f ) {
                                                return 60.0/2180.5;
                                            } else {
                                                if ( cl->size() <= 4.5f ) {
                                                    return 43.0/3258.6;
                                                } else {
                                                    return 34.0/1991.7;
                                                }
                                            }
                                        } else {
                                            return 101.0/2079.0;
                                        }
                                    } else {
                                        return 125.0/2643.4;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 298.0/3439.3;
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 26.0/2141.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0912296995521f ) {
                                                return 77.0/2280.0;
                                            } else {
                                                return 60.0/2363.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.205311000347f ) {
                                    if ( rdb0_last_touched_diff <= 8662.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3648.5f ) {
                                            return 108.0/1977.5;
                                        } else {
                                            return 171.0/1843.5;
                                        }
                                    } else {
                                        return 326.0/2407.9;
                                    }
                                } else {
                                    return 346.0/2462.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.372530728579f ) {
                                            return 432.0/1382.6;
                                        } else {
                                            return 440.0/1914.6;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 353.0/1705.4;
                                        } else {
                                            return 269.0/1620.2;
                                        }
                                    }
                                } else {
                                    return 784.0/2342.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    if ( rdb0_last_touched_diff <= 1843.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 15346.5f ) {
                                            return 138.0/2495.2;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.121244221926f ) {
                                                return 259.0/2107.4;
                                            } else {
                                                return 274.0/3096.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.407999455929f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.126313239336f ) {
                                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                    return 332.0/1711.5;
                                                } else {
                                                    return 342.0/1494.3;
                                                }
                                            } else {
                                                return 376.0/1354.2;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.11954690516f ) {
                                                return 292.0/1498.3;
                                            } else {
                                                return 282.0/1876.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0441960282624f ) {
                                        if ( cl->stats.glue_rel_long <= 0.369476020336f ) {
                                            return 270.0/2572.4;
                                        } else {
                                            return 156.0/1853.6;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 196.0/1914.6;
                                        } else {
                                            return 190.0/3780.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.412116706371f ) {
                                        return 507.0/2998.7;
                                    } else {
                                        return 259.0/2022.2;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 10465.5f ) {
                                        return 160.0/1839.4;
                                    } else {
                                        return 224.0/1640.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.200139090419f ) {
                                    if ( cl->stats.glue_rel_long <= 0.164591312408f ) {
                                        return 117.0/2921.6;
                                    } else {
                                        return 110.0/2062.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2476.5f ) {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0505306832492f ) {
                                                if ( rdb0_last_touched_diff <= 571.5f ) {
                                                    return 50.0/2401.8;
                                                } else {
                                                    return 80.0/1924.7;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 99.0/2091.2;
                                                } else {
                                                    if ( cl->stats.dump_number <= 23.5f ) {
                                                        if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 1654.0f ) {
                                                                return 32.0/2014.0;
                                                            } else {
                                                                return 49.0/2048.6;
                                                            }
                                                        } else {
                                                            if ( cl->stats.rdb1_last_touched_diff <= 1184.5f ) {
                                                                if ( cl->stats.glue_rel_long <= 0.338757514954f ) {
                                                                    return 24.0/2091.2;
                                                                } else {
                                                                    return 7.0/2554.1;
                                                                }
                                                            } else {
                                                                return 28.0/2121.6;
                                                            }
                                                        }
                                                    } else {
                                                        return 78.0/3812.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 66.0/2056.7;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.143824607134f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.108703836799f ) {
                                                return 199.0/3333.7;
                                            } else {
                                                return 91.0/2310.5;
                                            }
                                        } else {
                                            return 208.0/2797.7;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 443.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.354861110449f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            return 639.0/2550.0;
                                        } else {
                                            return 308.0/2139.9;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.113641247153f ) {
                                            return 75.0/2448.5;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0906723290682f ) {
                                                    return 202.0/1910.5;
                                                } else {
                                                    return 162.0/2097.3;
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 31.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.106599017978f ) {
                                                        return 170.0/3711.4;
                                                    } else {
                                                        return 64.0/2334.8;
                                                    }
                                                } else {
                                                    return 23.0/2030.3;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        if ( rdb0_last_touched_diff <= 9996.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                return 326.0/2848.5;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.155892834067f ) {
                                                    return 143.0/2706.4;
                                                } else {
                                                    return 88.0/3360.1;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 608.0/1567.4;
                                            } else {
                                                return 490.0/2194.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0207939501852f ) {
                                            if ( rdb0_last_touched_diff <= 2882.0f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3010.5f ) {
                                                    return 117.0/3561.1;
                                                } else {
                                                    return 141.0/1959.2;
                                                }
                                            } else {
                                                return 251.0/2146.0;
                                            }
                                        } else {
                                            return 305.0/3209.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 23658.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.686158359051f ) {
                                        return 337.0/2816.0;
                                    } else {
                                        return 216.0/2446.5;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.690627574921f ) {
                                        return 443.0/2154.1;
                                    } else {
                                        return 434.0/1624.2;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 5512.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.690584301949f ) {
                                        return 288.0/1754.2;
                                    } else {
                                        return 454.0/1932.8;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                            if ( cl->size() <= 10.5f ) {
                                                return 185.0/2154.1;
                                            } else {
                                                return 318.0/2608.9;
                                            }
                                        } else {
                                            return 94.0/2050.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.113962993026f ) {
                                            return 90.0/1971.4;
                                        } else {
                                            return 92.0/3709.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.851777672768f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 527.0/2659.7;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 7620.5f ) {
                                            return 482.0/2405.9;
                                        } else {
                                            return 425.0/1175.5;
                                        }
                                    }
                                } else {
                                    return 868.0/2251.6;
                                }
                            }
                        }
                    } else {
                        return 429.0/1480.1;
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.436699330807f ) {
                        if ( cl->stats.glue_rel_long <= 0.921377241611f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 857.0/2121.6;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 44.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.70908755064f ) {
                                        if ( rdb0_last_touched_diff <= 9235.0f ) {
                                            return 280.0/1514.6;
                                        } else {
                                            return 469.0/1815.1;
                                        }
                                    } else {
                                        return 445.0/1555.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.668658673763f ) {
                                        return 487.0/1654.7;
                                    } else {
                                        return 488.0/1358.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 58.5f ) {
                                return 462.0/1195.8;
                            } else {
                                return 556.0/1082.1;
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 10.5f ) {
                            if ( rdb0_last_touched_diff <= 12587.5f ) {
                                return 634.0/1916.6;
                            } else {
                                return 625.0/1043.6;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.575971245766f ) {
                                return 885.0/1695.3;
                            } else {
                                if ( cl->stats.dump_number <= 1.5f ) {
                                    return 844.0/489.3;
                                } else {
                                    return 727.0/1335.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 239.0/1833.3;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.394444465637f ) {
                                    return 171.0/2014.0;
                                } else {
                                    return 211.0/1821.2;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2910.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 28563.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.748044013977f ) {
                                        return 188.0/1737.9;
                                    } else {
                                        return 264.0/1577.5;
                                    }
                                } else {
                                    return 316.0/1437.4;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 24952.0f ) {
                                    return 547.0/2554.1;
                                } else {
                                    return 434.0/1226.3;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 2442.0f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1679.5f ) {
                                    return 81.0/1991.7;
                                } else {
                                    return 138.0/1841.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.209582597017f ) {
                                        return 152.0/2079.0;
                                    } else {
                                        return 169.0/2852.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 14.5f ) {
                                        if ( rdb0_last_touched_diff <= 330.0f ) {
                                            return 45.0/2012.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.703526377678f ) {
                                                return 75.0/2235.3;
                                            } else {
                                                return 118.0/2208.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 26.5f ) {
                                            return 47.0/1983.6;
                                        } else {
                                            return 49.0/3697.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.42684340477f ) {
                                if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                        return 288.0/2237.4;
                                    } else {
                                        return 219.0/2192.7;
                                    }
                                } else {
                                    return 142.0/2068.9;
                                }
                            } else {
                                return 237.0/1673.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 4.01868152618f ) {
                if ( rdb0_last_touched_diff <= 10792.0f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.671731114388f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0794470459223f ) {
                                    return 559.0/2962.2;
                                } else {
                                    return 401.0/1632.3;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 6964.0f ) {
                                    if ( rdb0_last_touched_diff <= 3585.0f ) {
                                        return 119.0/2621.1;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            return 117.0/1989.7;
                                        } else {
                                            return 167.0/1758.2;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 8872.0f ) {
                                        return 203.0/2131.8;
                                    } else {
                                        return 268.0/2152.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    return 496.0/1559.3;
                                } else {
                                    return 360.0/1476.0;
                                }
                            } else {
                                return 272.0/2818.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 28215.0f ) {
                            return 606.0/2495.2;
                        } else {
                            return 550.0/1017.2;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.291523873806f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0482085458934f ) {
                                    return 608.0/1155.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.148310601711f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                return 415.0/1681.1;
                                            } else {
                                                return 444.0/1530.8;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.493833184242f ) {
                                                return 624.0/2182.6;
                                            } else {
                                                return 441.0/1277.0;
                                            }
                                        }
                                    } else {
                                        return 717.0/1705.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.827597439289f ) {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 1127.0/1360.3;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.118777416646f ) {
                                            return 544.0/964.4;
                                        } else {
                                            return 793.0/1876.0;
                                        }
                                    }
                                } else {
                                    return 955.0/1005.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                return 551.0/1658.7;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.723857760429f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.103594660759f ) {
                                        return 590.0/2917.5;
                                    } else {
                                        return 462.0/3021.1;
                                    }
                                } else {
                                    return 385.0/1319.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 14.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.708088994026f ) {
                                return 511.0/1506.5;
                            } else {
                                return 587.0/1045.6;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.545317828655f ) {
                                return 626.0/911.6;
                            } else {
                                return 1206.0/952.2;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 0.968892335892f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            return 1484.0/933.9;
                        } else {
                            return 706.0/1449.6;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.741134345531f ) {
                            return 409.0/1297.4;
                        } else {
                            return 509.0/1110.6;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 10791.5f ) {
                        return 575.0/966.4;
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 16.9236106873f ) {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                return 1028.0/290.3;
                            } else {
                                return 787.0/670.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.23361349106f ) {
                                return 1365.0/456.8;
                            } else {
                                return 1792.0/341.1;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue_rel_long <= 0.833416700363f ) {
            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                if ( cl->size() <= 8.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 64179.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.129447162151f ) {
                            if ( cl->stats.size_rel <= 0.0673636421561f ) {
                                return 750.0/2239.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 38406.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.382044285536f ) {
                                                return 410.0/1226.3;
                                            } else {
                                                return 368.0/1360.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0511516481638f ) {
                                                if ( cl->stats.glue_rel_long <= 0.366643667221f ) {
                                                    return 570.0/1376.5;
                                                } else {
                                                    return 577.0/1161.3;
                                                }
                                            } else {
                                                return 458.0/1435.4;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.400320529938f ) {
                                            return 485.0/1145.1;
                                        } else {
                                            return 764.0/1526.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0287166014314f ) {
                                        return 661.0/858.8;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.42277354002f ) {
                                            return 671.0/1238.5;
                                        } else {
                                            return 812.0/1161.3;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 583.0/1764.3;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 188424.0f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0340462997556f ) {
                                    return 657.0/1092.3;
                                } else {
                                    return 800.0/1143.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.371058791876f ) {
                                    if ( cl->stats.size_rel <= 0.144810318947f ) {
                                        return 649.0/879.1;
                                    } else {
                                        return 665.0/702.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        return 1121.0/1214.1;
                                    } else {
                                        return 784.0/659.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0193254668266f ) {
                                return 808.0/722.8;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 348699.0f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0565120838583f ) {
                                        return 788.0/511.6;
                                    } else {
                                        return 726.0/586.8;
                                    }
                                } else {
                                    return 850.0/402.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        if ( rdb0_last_touched_diff <= 47628.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 26798.5f ) {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    return 641.0/1242.5;
                                } else {
                                    return 612.0/1616.1;
                                }
                            } else {
                                return 821.0/1364.4;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 1256.0/1583.6;
                            } else {
                                return 1186.0/1120.7;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 894.0/1005.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 179685.0f ) {
                                    return 1166.0/994.8;
                                } else {
                                    return 931.0/367.5;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 21.5f ) {
                                return 796.0/576.6;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.559281229973f ) {
                                    return 950.0/485.2;
                                } else {
                                    return 958.0/367.5;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 94015.0f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 40640.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 1010.0/1829.3;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.257301867008f ) {
                                    return 773.0/1776.5;
                                } else {
                                    return 456.0/1309.5;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 55.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.580538153648f ) {
                                    if ( rdb0_last_touched_diff <= 36103.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 615.0/913.6;
                                        } else {
                                            return 832.0/1807.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0988432765007f ) {
                                            return 747.0/1187.7;
                                        } else {
                                            return 704.0/712.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.871312499046f ) {
                                        if ( rdb0_last_touched_diff <= 28416.0f ) {
                                            return 573.0/968.4;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.132961764932f ) {
                                                return 693.0/763.4;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                    return 808.0/1258.8;
                                                } else {
                                                    return 849.0/854.7;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1102.0/1015.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.601507306099f ) {
                                    return 1158.0/1325.8;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 716.0/751.2;
                                    } else {
                                        return 1471.0/828.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.182245403528f ) {
                                return 688.0/1139.0;
                            } else {
                                return 703.0/692.3;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->size() <= 12.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 56786.0f ) {
                                        return 633.0/785.7;
                                    } else {
                                        return 858.0/722.8;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.668367981911f ) {
                                            return 1004.0/773.5;
                                        } else {
                                            return 812.0/393.9;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 1070.0/493.4;
                                        } else {
                                            if ( cl->stats.glue <= 10.5f ) {
                                                return 1101.0/467.0;
                                            } else {
                                                return 974.0/196.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.301339447498f ) {
                                    return 833.0/1216.1;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.744116842747f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.166419744492f ) {
                                            return 788.0/970.5;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.314446389675f ) {
                                                return 865.0/676.1;
                                            } else {
                                                return 689.0/726.8;
                                            }
                                        }
                                    } else {
                                        return 719.0/588.8;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.28603720665f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.134575724602f ) {
                                return 799.0/531.9;
                            } else {
                                return 1286.0/1287.2;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 64.5f ) {
                                return 1287.0/789.8;
                            } else {
                                return 1160.0/558.3;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            if ( cl->size() <= 46.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 862.0/548.2;
                                } else {
                                    if ( cl->stats.dump_number <= 26.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.617408275604f ) {
                                            return 1440.0/958.3;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 55.5f ) {
                                                return 843.0/428.4;
                                            } else {
                                                return 1551.0/509.6;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 249267.0f ) {
                                            return 781.0/444.6;
                                        } else {
                                            return 1547.0/347.2;
                                        }
                                    }
                                }
                            } else {
                                return 1485.0/412.1;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.874760270119f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 154119.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.728821754456f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.280954629183f ) {
                                            return 806.0/550.2;
                                        } else {
                                            return 1005.0/436.5;
                                        }
                                    } else {
                                        return 903.0/284.2;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            return 1797.0/493.4;
                                        } else {
                                            return 1234.0/237.5;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 1114.0/448.7;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 39.5f ) {
                                                return 943.0/314.7;
                                            } else {
                                                return 1044.0/282.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.598278284073f ) {
                                    return 1071.0/272.1;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.785268425941f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 254051.5f ) {
                                            return 1501.0/333.0;
                                        } else {
                                            return 1016.0/105.6;
                                        }
                                    } else {
                                        return 955.0/227.4;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.size_rel <= 0.794964432716f ) {
                if ( cl->stats.num_total_lits_antecedents <= 120.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                        if ( rdb0_last_touched_diff <= 66870.0f ) {
                            return 1093.0/1871.9;
                        } else {
                            return 1551.0/936.0;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 63417.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 5.3466668129f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    if ( rdb0_last_touched_diff <= 39064.5f ) {
                                        return 835.0/1447.6;
                                    } else {
                                        return 749.0/828.4;
                                    }
                                } else {
                                    return 1346.0/1120.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 31236.5f ) {
                                    return 828.0/619.2;
                                } else {
                                    return 911.0/432.5;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 1470.0/897.4;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 29.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.04993438721f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 166644.0f ) {
                                            return 1250.0/720.8;
                                        } else {
                                            return 1180.0/320.8;
                                        }
                                    } else {
                                        return 1547.0/495.4;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.208919107914f ) {
                                        return 930.0/150.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 124539.5f ) {
                                            return 872.0/326.9;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 5.84500026703f ) {
                                                return 887.0/263.9;
                                            } else {
                                                return 955.0/176.6;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.987022519112f ) {
                            return 1223.0/674.1;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 7.2474489212f ) {
                                return 847.0/416.2;
                            } else {
                                if ( cl->stats.glue <= 17.5f ) {
                                    return 922.0/239.6;
                                } else {
                                    return 1219.0/154.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.688096702099f ) {
                            return 872.0/361.4;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 32311.5f ) {
                                    return 1104.0/306.6;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 5.1875500679f ) {
                                        return 963.0/247.7;
                                    } else {
                                        if ( cl->stats.glue <= 17.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 154.5f ) {
                                                return 957.0/178.7;
                                            } else {
                                                return 1375.0/160.4;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.07652759552f ) {
                                                return 965.0/117.8;
                                            } else {
                                                return 1436.0/81.2;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 1.05431962013f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        return 1095.0/249.7;
                                    } else {
                                        return 1125.0/127.9;
                                    }
                                } else {
                                    return 1549.0/416.2;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 1.04140663147f ) {
                    if ( rdb0_last_touched_diff <= 75902.0f ) {
                        if ( rdb0_last_touched_diff <= 33996.5f ) {
                            return 1302.0/998.9;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 277.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.394177377224f ) {
                                    return 1407.0/854.7;
                                } else {
                                    return 1684.0/690.3;
                                }
                            } else {
                                return 1029.0/249.7;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.191386669874f ) {
                                return 1286.0/434.5;
                            } else {
                                if ( rdb0_last_touched_diff <= 161284.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.919284760952f ) {
                                        return 974.0/357.3;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 13.0282649994f ) {
                                            return 1289.0/288.3;
                                        } else {
                                            return 953.0/142.1;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 1515.0/156.3;
                                    } else {
                                        return 1180.0/241.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 143252.5f ) {
                                return 927.0/194.9;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                    if ( cl->stats.size_rel <= 1.13315033913f ) {
                                        return 941.0/138.1;
                                    } else {
                                        return 1049.0/103.5;
                                    }
                                } else {
                                    return 980.0/85.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 36654.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 359.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 114.5f ) {
                                    return 914.0/580.7;
                                } else {
                                    return 1196.0/489.3;
                                }
                            } else {
                                return 1028.0/172.6;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.423255980015f ) {
                                return 1461.0/395.9;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 61531.0f ) {
                                    return 1212.0/190.8;
                                } else {
                                    return 1779.0/172.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.588204503059f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.743245124817f ) {
                                if ( cl->stats.size_rel <= 1.95069694519f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 255.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.84968757629f ) {
                                                return 1645.0/716.7;
                                            } else {
                                                if ( cl->stats.glue <= 13.5f ) {
                                                    return 1031.0/300.5;
                                                } else {
                                                    if ( cl->stats.dump_number <= 13.5f ) {
                                                        return 1144.0/207.1;
                                                    } else {
                                                        return 1201.0/162.4;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 938.0/129.9;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 7.01149368286f ) {
                                            return 942.0/223.3;
                                        } else {
                                            return 1046.0/73.1;
                                        }
                                    }
                                } else {
                                    return 988.0/79.2;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 91851.5f ) {
                                    return 1382.0/227.4;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 245.5f ) {
                                        return 1179.0/132.0;
                                    } else {
                                        return 1099.0/65.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 87640.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.941904425621f ) {
                                    return 1880.0/355.3;
                                } else {
                                    if ( cl->stats.glue <= 15.5f ) {
                                        return 1276.0/215.2;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.27090013027f ) {
                                            return 1046.0/132.0;
                                        } else {
                                            return 1632.0/127.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 200.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 11.7033176422f ) {
                                            return 1208.0/125.9;
                                        } else {
                                            return 1421.0/75.1;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 1.90314984322f ) {
                                            return 1189.0/26.4;
                                        } else {
                                            return 1050.0/75.1;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 29.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                            return 1115.0/243.6;
                                        } else {
                                            return 1125.0/107.6;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.31173062325f ) {
                                            return 1122.0/152.3;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                return 1553.0/134.0;
                                            } else {
                                                return 1774.0/40.6;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_2(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.glue <= 9.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 24378.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                        if ( cl->stats.size_rel <= 0.561517000198f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0251289829612f ) {
                                return 559.0/1463.8;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0754289478064f ) {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( rdb0_last_touched_diff <= 15054.5f ) {
                                                return 295.0/1606.0;
                                            } else {
                                                return 509.0/1892.2;
                                            }
                                        } else {
                                            return 424.0/1315.6;
                                        }
                                    } else {
                                        return 526.0/1713.6;
                                    }
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->size() <= 6.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.324601829052f ) {
                                                        return 337.0/1449.6;
                                                    } else {
                                                        return 502.0/2428.2;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.200947999954f ) {
                                                        return 613.0/1815.1;
                                                    } else {
                                                        return 368.0/1642.5;
                                                    }
                                                }
                                            } else {
                                                return 590.0/1977.5;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.386946499348f ) {
                                                return 401.0/3065.7;
                                            } else {
                                                return 443.0/2836.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 33.5f ) {
                                            if ( cl->stats.dump_number <= 2.5f ) {
                                                return 419.0/1431.4;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.398113310337f ) {
                                                    return 631.0/2353.1;
                                                } else {
                                                    return 474.0/2081.0;
                                                }
                                            }
                                        } else {
                                            return 557.0/1612.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 632.0/1683.1;
                        }
                    } else {
                        if ( cl->stats.glue <= 4.5f ) {
                            if ( cl->size() <= 6.5f ) {
                                return 363.0/1977.5;
                            } else {
                                if ( rdb0_last_touched_diff <= 17468.5f ) {
                                    return 414.0/1429.3;
                                } else {
                                    return 496.0/1086.2;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 13025.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.702720522881f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->size() <= 9.5f ) {
                                            return 399.0/1472.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.376038104296f ) {
                                                if ( cl->stats.dump_number <= 11.5f ) {
                                                    return 740.0/1668.9;
                                                } else {
                                                    return 605.0/1206.0;
                                                }
                                            } else {
                                                return 575.0/984.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.522814333439f ) {
                                            return 594.0/3015.0;
                                        } else {
                                            return 350.0/1360.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 688.0/1551.1;
                                        } else {
                                            if ( cl->stats.dump_number <= 3.5f ) {
                                                return 649.0/789.8;
                                            } else {
                                                return 518.0/1072.0;
                                            }
                                        }
                                    } else {
                                        return 488.0/1853.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        return 588.0/1472.0;
                                    } else {
                                        return 570.0/950.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 720.0/1417.1;
                                    } else {
                                        return 808.0/1011.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->size() <= 7.5f ) {
                            if ( rdb0_last_touched_diff <= 54929.0f ) {
                                if ( cl->stats.size_rel <= 0.24682597816f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.352837443352f ) {
                                        return 547.0/1179.6;
                                    } else {
                                        return 546.0/1547.1;
                                    }
                                } else {
                                    return 414.0/1313.6;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 70639.5f ) {
                                    return 537.0/1080.1;
                                } else {
                                    return 765.0/1240.5;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.160847216845f ) {
                                    return 532.0/990.8;
                                } else {
                                    return 536.0/1116.7;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.115911990404f ) {
                                    return 676.0/970.5;
                                } else {
                                    return 818.0/1007.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 41.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.324999988079f ) {
                                    return 867.0/1295.3;
                                } else {
                                    return 908.0/1183.7;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    return 1116.0/919.7;
                                } else {
                                    return 1084.0/1163.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 755.0/830.4;
                            } else {
                                return 1049.0/590.8;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.565977692604f ) {
                        if ( rdb0_last_touched_diff <= 61458.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.938995718956f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.749074697495f ) {
                                            return 738.0/1151.2;
                                        } else {
                                            return 807.0/958.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 20999.5f ) {
                                            return 911.0/2001.9;
                                        } else {
                                            return 648.0/1067.9;
                                        }
                                    }
                                } else {
                                    return 1040.0/1061.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.28305768967f ) {
                                        return 810.0/958.3;
                                    } else {
                                        return 1442.0/972.5;
                                    }
                                } else {
                                    return 933.0/574.6;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 1016.0/834.4;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 73.5f ) {
                                    return 885.0/487.3;
                                } else {
                                    return 1090.0/294.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.00552356243f ) {
                            if ( cl->stats.glue <= 16.5f ) {
                                if ( rdb0_last_touched_diff <= 50052.0f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 896.0/1102.4;
                                    } else {
                                        return 736.0/574.6;
                                    }
                                } else {
                                    return 981.0/464.9;
                                }
                            } else {
                                return 1011.0/491.3;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.0426902771f ) {
                                    return 986.0/328.9;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 393.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 29202.0f ) {
                                            return 1673.0/501.5;
                                        } else {
                                            return 991.0/121.8;
                                        }
                                    } else {
                                        if ( cl->size() <= 101.5f ) {
                                            return 1248.0/207.1;
                                        } else {
                                            return 991.0/54.8;
                                        }
                                    }
                                }
                            } else {
                                return 850.0/507.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.930577516556f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.365779578686f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.200567483902f ) {
                                return 509.0/2008.0;
                            } else {
                                return 468.0/1419.2;
                            }
                        } else {
                            return 496.0/1360.3;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.73760151863f ) {
                            return 501.0/1145.1;
                        } else {
                            return 861.0/1173.5;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.557245194912f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0307355709374f ) {
                                    if ( cl->stats.size_rel <= 0.200269162655f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                            if ( rdb0_last_touched_diff <= 3815.0f ) {
                                                return 278.0/2028.3;
                                            } else {
                                                return 342.0/1660.8;
                                            }
                                        } else {
                                            return 202.0/2649.5;
                                        }
                                    } else {
                                        return 162.0/1766.3;
                                    }
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2359.5f ) {
                                            return 140.0/2858.6;
                                        } else {
                                            return 275.0/3479.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.167727798223f ) {
                                            return 254.0/3179.4;
                                        } else {
                                            return 190.0/1681.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 5260.0f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.123533092439f ) {
                                        return 114.0/1943.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 10.5f ) {
                                            if ( cl->stats.size_rel <= 0.217445760965f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.311122655869f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                                        return 113.0/2070.9;
                                                    } else {
                                                        return 88.0/3963.1;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 1184.5f ) {
                                                        return 18.0/2740.9;
                                                    } else {
                                                        return 86.0/2131.8;
                                                    }
                                                }
                                            } else {
                                                return 50.0/3845.4;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                                return 165.0/2635.3;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                                    if ( rdb0_last_touched_diff <= 2242.0f ) {
                                                        return 73.0/2600.8;
                                                    } else {
                                                        return 106.0/1945.0;
                                                    }
                                                } else {
                                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0612008534372f ) {
                                                            return 85.0/2430.3;
                                                        } else {
                                                            return 34.0/2497.2;
                                                        }
                                                    } else {
                                                        if ( cl->size() <= 6.5f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0261593051255f ) {
                                                                return 54.0/1999.8;
                                                            } else {
                                                                return 41.0/3796.6;
                                                            }
                                                        } else {
                                                            return 63.0/2363.3;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2078.5f ) {
                                        if ( cl->stats.dump_number <= 25.5f ) {
                                            return 108.0/2314.5;
                                        } else {
                                            return 66.0/1959.2;
                                        }
                                    } else {
                                        return 210.0/2560.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.551946759224f ) {
                                if ( cl->stats.glue_rel_queue <= 0.524967193604f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 6476.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1118.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 83.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0513305962086f ) {
                                                    return 60.0/2533.8;
                                                } else {
                                                    return 79.0/2304.4;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0920308753848f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0417960323393f ) {
                                                        return 141.0/3374.3;
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                                            return 172.0/1825.2;
                                                        } else {
                                                            return 77.0/1967.3;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.494897961617f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.211225479841f ) {
                                                            return 111.0/2543.9;
                                                        } else {
                                                            return 106.0/3794.6;
                                                        }
                                                    } else {
                                                        return 95.0/2026.2;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.154380172491f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.109706223011f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                        return 306.0/3029.2;
                                                    } else {
                                                        return 138.0/3461.6;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.403042912483f ) {
                                                        return 129.0/3398.7;
                                                    } else {
                                                        return 116.0/1991.7;
                                                    }
                                                }
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 322.0/3445.4;
                                                } else {
                                                    return 183.0/3092.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1253.0f ) {
                                            return 218.0/3017.0;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                if ( rdb0_last_touched_diff <= 4435.5f ) {
                                                    return 235.0/1656.7;
                                                } else {
                                                    return 289.0/1575.5;
                                                }
                                            } else {
                                                return 262.0/2158.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 3219.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.111394673586f ) {
                                            return 111.0/1932.8;
                                        } else {
                                            return 133.0/2988.6;
                                        }
                                    } else {
                                        return 338.0/2269.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 441.0/2249.6;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                        return 230.0/2586.6;
                                    } else {
                                        return 122.0/3654.5;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.493782371283f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                if ( rdb0_last_touched_diff <= 3607.0f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0321357399225f ) {
                                                return 225.0/1583.6;
                                            } else {
                                                return 173.0/2767.3;
                                            }
                                        } else {
                                            return 229.0/2044.5;
                                        }
                                    } else {
                                        return 124.0/2962.2;
                                    }
                                } else {
                                    return 551.0/2765.2;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 749.0/2639.4;
                                } else {
                                    if ( cl->stats.size_rel <= 0.146056115627f ) {
                                        return 161.0/2200.8;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.256392240524f ) {
                                            return 169.0/1772.4;
                                        } else {
                                            return 199.0/2395.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 1771.5f ) {
                                return 194.0/2117.6;
                            } else {
                                return 469.0/1813.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 4864.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                            return 287.0/1882.1;
                        } else {
                            return 117.0/2643.4;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 8706.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                return 403.0/2803.8;
                            } else {
                                return 158.0/2182.6;
                            }
                        } else {
                            return 480.0/1603.9;
                        }
                    }
                }
            } else {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                return 305.0/1910.5;
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.dump_number <= 14.5f ) {
                                        return 509.0/2160.2;
                                    } else {
                                        return 346.0/1670.9;
                                    }
                                } else {
                                    return 521.0/2554.1;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3784.0f ) {
                                if ( cl->stats.size_rel <= 0.33752784133f ) {
                                    return 284.0/3045.4;
                                } else {
                                    return 119.0/2493.2;
                                }
                            } else {
                                return 274.0/1863.8;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 3246.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.954622745514f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 239.0/3254.5;
                                    } else {
                                        return 119.0/3400.7;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 927.5f ) {
                                        return 60.0/3002.8;
                                    } else {
                                        return 108.0/3086.0;
                                    }
                                }
                            } else {
                                return 141.0/2588.6;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2881.5f ) {
                                return 179.0/3303.3;
                            } else {
                                return 217.0/2079.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.186427950859f ) {
                                    if ( rdb0_last_touched_diff <= 2475.0f ) {
                                        return 394.0/2308.4;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.461375832558f ) {
                                            return 447.0/1212.1;
                                        } else {
                                            return 738.0/2600.8;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 4280.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 11823.0f ) {
                                            return 332.0/1504.4;
                                        } else {
                                            return 388.0/1372.5;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.788892328739f ) {
                                            return 416.0/1268.9;
                                        } else {
                                            return 509.0/1118.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.369216710329f ) {
                                    return 636.0/1281.1;
                                } else {
                                    return 588.0/899.4;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.93855291605f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 37718.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 164.5f ) {
                                        if ( cl->stats.dump_number <= 2.5f ) {
                                            return 293.0/2121.6;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.162380233407f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                    return 183.0/1995.8;
                                                } else {
                                                    return 164.0/2036.4;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 2788.5f ) {
                                                    return 223.0/2954.1;
                                                } else {
                                                    return 239.0/1575.5;
                                                }
                                            }
                                        }
                                    } else {
                                        return 230.0/1630.3;
                                    }
                                } else {
                                    return 391.0/2442.4;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2486.5f ) {
                                    return 304.0/3075.9;
                                } else {
                                    return 452.0/1973.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 165.5f ) {
                            if ( rdb0_last_touched_diff <= 4824.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0707828775048f ) {
                                    return 213.0/3851.4;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0744855776429f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.144690960646f ) {
                                            return 77.0/1987.6;
                                        } else {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.272177159786f ) {
                                                    return 41.0/1953.1;
                                                } else {
                                                    return 76.0/2180.5;
                                                }
                                            } else {
                                                return 51.0/3396.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                return 185.0/1863.8;
                                            } else {
                                                return 99.0/1953.1;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                                return 88.0/2008.0;
                                            } else {
                                                return 80.0/3682.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                    return 428.0/3236.3;
                                } else {
                                    return 160.0/1800.9;
                                }
                            }
                        } else {
                            return 225.0/2474.9;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.glue <= 7.5f ) {
            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.14704452455f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                            if ( cl->stats.size_rel <= 0.116746984422f ) {
                                return 780.0/1770.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 19190.5f ) {
                                    return 501.0/1662.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0363600179553f ) {
                                        return 988.0/1193.8;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.417087316513f ) {
                                            return 472.0/1072.0;
                                        } else {
                                            return 841.0/1273.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    return 836.0/1878.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 54456.0f ) {
                                        return 522.0/1110.6;
                                    } else {
                                        return 902.0/962.4;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 1027.0/1407.0;
                                } else {
                                    return 999.0/956.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 162024.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 42654.5f ) {
                                    return 461.0/1161.3;
                                } else {
                                    return 589.0/879.1;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 52403.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.574760913849f ) {
                                        return 908.0/1425.3;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.346154868603f ) {
                                            return 786.0/791.8;
                                        } else {
                                            return 639.0/992.8;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 12.5f ) {
                                        return 897.0/1165.4;
                                    } else {
                                        return 1195.0/732.9;
                                    }
                                }
                            }
                        } else {
                            return 844.0/416.2;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.515742540359f ) {
                        if ( rdb0_last_touched_diff <= 11158.5f ) {
                            return 171.0/1784.6;
                        } else {
                            return 441.0/2087.1;
                        }
                    } else {
                        return 455.0/1971.4;
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                    if ( cl->stats.dump_number <= 6.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.510421156883f ) {
                            return 880.0/1871.9;
                        } else {
                            return 1277.0/1553.2;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.478298604488f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.dump_number <= 20.5f ) {
                                        return 894.0/1279.1;
                                    } else {
                                        return 713.0/769.5;
                                    }
                                } else {
                                    return 953.0/929.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 20.5f ) {
                                    return 1173.0/1051.7;
                                } else {
                                    return 1135.0/564.4;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.637806117535f ) {
                                return 1367.0/996.9;
                            } else {
                                return 1285.0/643.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                        if ( cl->size() <= 8.5f ) {
                            if ( rdb0_last_touched_diff <= 90859.5f ) {
                                return 661.0/1236.4;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 245584.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.439453542233f ) {
                                        return 978.0/1112.6;
                                    } else {
                                        return 957.0/751.2;
                                    }
                                } else {
                                    return 1263.0/804.0;
                                }
                            }
                        } else {
                            return 1402.0/791.8;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 60486.0f ) {
                            return 1197.0/1360.3;
                        } else {
                            if ( cl->stats.size_rel <= 0.286056816578f ) {
                                if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                    return 1080.0/678.1;
                                } else {
                                    return 900.0/676.1;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.69765663147f ) {
                                    if ( rdb0_last_touched_diff <= 245554.0f ) {
                                        if ( cl->size() <= 11.5f ) {
                                            return 901.0/531.9;
                                        } else {
                                            return 1011.0/400.0;
                                        }
                                    } else {
                                        return 1296.0/383.7;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        return 980.0/282.2;
                                    } else {
                                        return 994.0/156.3;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 22993.0f ) {
                if ( cl->stats.num_overlap_literals <= 72.5f ) {
                    if ( cl->stats.dump_number <= 3.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.791231453419f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 9776.5f ) {
                                return 514.0/1264.9;
                            } else {
                                return 654.0/885.2;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.glue <= 11.5f ) {
                                    return 805.0/485.2;
                                } else {
                                    return 1297.0/515.7;
                                }
                            } else {
                                return 1088.0/1118.7;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.708478569984f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 11350.5f ) {
                                return 387.0/1695.3;
                            } else {
                                return 559.0/1457.7;
                            }
                        } else {
                            return 727.0/1344.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.898732185364f ) {
                        return 1148.0/1193.8;
                    } else {
                        if ( cl->stats.num_overlap_literals <= 235.5f ) {
                            if ( rdb0_last_touched_diff <= 18671.0f ) {
                                return 899.0/479.1;
                            } else {
                                return 1799.0/507.6;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 2.4293217659f ) {
                                return 1842.0/367.5;
                            } else {
                                return 1214.0/176.6;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.418490439653f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 140818.5f ) {
                        if ( cl->stats.num_overlap_literals <= 19.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.664656281471f ) {
                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.480522066355f ) {
                                        return 611.0/881.1;
                                    } else {
                                        return 1137.0/968.4;
                                    }
                                } else {
                                    return 1047.0/826.3;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0711433142424f ) {
                                    return 1091.0/471.0;
                                } else {
                                    if ( cl->size() <= 11.5f ) {
                                        return 732.0/765.4;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 59755.5f ) {
                                            if ( cl->stats.dump_number <= 5.5f ) {
                                                return 890.0/505.5;
                                            } else {
                                                return 959.0/976.6;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0801301896572f ) {
                                                return 1183.0/649.7;
                                            } else {
                                                return 1728.0/609.1;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.690900504589f ) {
                                if ( rdb0_last_touched_diff <= 68028.5f ) {
                                    return 721.0/735.0;
                                } else {
                                    return 1319.0/649.7;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.192632377148f ) {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 66855.5f ) {
                                            return 1631.0/326.9;
                                        } else {
                                            return 969.0/89.3;
                                        }
                                    } else {
                                        return 1034.0/371.5;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.906324505806f ) {
                                            return 947.0/402.0;
                                        } else {
                                            return 1719.0/434.5;
                                        }
                                    } else {
                                        return 953.0/544.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.14003777504f ) {
                            if ( cl->stats.size_rel <= 0.862067699432f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    return 905.0/477.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 288300.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.813165545464f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.28146266937f ) {
                                                if ( cl->stats.glue <= 9.5f ) {
                                                    return 860.0/381.7;
                                                } else {
                                                    return 1049.0/540.1;
                                                }
                                            } else {
                                                return 1151.0/351.2;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                                                return 938.0/292.4;
                                            } else {
                                                return 1054.0/259.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 621747.5f ) {
                                            if ( rdb0_last_touched_diff <= 426925.5f ) {
                                                return 1632.0/389.8;
                                            } else {
                                                return 1230.0/196.9;
                                            }
                                        } else {
                                            return 974.0/330.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                    return 1825.0/373.6;
                                } else {
                                    return 1277.0/207.1;
                                }
                            }
                        } else {
                            if ( cl->size() <= 28.5f ) {
                                return 1073.0/109.6;
                            } else {
                                return 1324.0/211.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 172.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 167.5f ) {
                                if ( cl->size() <= 15.5f ) {
                                    return 840.0/578.6;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.565275907516f ) {
                                        return 860.0/438.5;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.04793214798f ) {
                                            return 836.0/363.4;
                                        } else {
                                            return 947.0/282.2;
                                        }
                                    }
                                }
                            } else {
                                return 1191.0/306.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 53775.0f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 1.096108675f ) {
                                    if ( cl->stats.glue_rel_long <= 0.927495241165f ) {
                                        return 779.0/467.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 1.17165517807f ) {
                                            return 996.0/363.4;
                                        } else {
                                            return 1019.0/199.0;
                                        }
                                    }
                                } else {
                                    return 933.0/215.2;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.641904532909f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 187506.5f ) {
                                        return 1604.0/536.0;
                                    } else {
                                        return 1036.0/280.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.818897783756f ) {
                                        if ( rdb0_last_touched_diff <= 241595.5f ) {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                return 1297.0/320.8;
                                            } else {
                                                return 891.0/324.8;
                                            }
                                        } else {
                                            return 1308.0/282.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.47316932678f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 139003.5f ) {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    if ( cl->size() <= 30.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.677938461304f ) {
                                                            return 1292.0/239.6;
                                                        } else {
                                                            return 1761.0/176.6;
                                                        }
                                                    } else {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            return 983.0/201.0;
                                                        } else {
                                                            return 1105.0/237.5;
                                                        }
                                                    }
                                                } else {
                                                    return 1085.0/416.2;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.378723084927f ) {
                                                    return 1894.0/363.4;
                                                } else {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 5.20499992371f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                            return 1660.0/333.0;
                                                        } else {
                                                            return 1996.0/192.9;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.05388665199f ) {
                                                            return 1923.0/243.6;
                                                        } else {
                                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                                return 1575.0/182.7;
                                                            } else {
                                                                return 1593.0/101.5;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 14.5029582977f ) {
                                                return 1404.0/172.6;
                                            } else {
                                                return 1903.0/87.3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 16.5f ) {
                            if ( cl->stats.glue <= 11.5f ) {
                                return 1117.0/387.8;
                            } else {
                                return 1024.0/154.3;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.965999960899f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 509.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.946559429169f ) {
                                        return 898.0/349.2;
                                    } else {
                                        if ( cl->stats.glue <= 20.5f ) {
                                            return 1269.0/182.7;
                                        } else {
                                            return 1398.0/138.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 18.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.05797731876f ) {
                                            return 1168.0/164.5;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.84421205521f ) {
                                                return 1065.0/73.1;
                                            } else {
                                                return 966.0/93.4;
                                            }
                                        }
                                    } else {
                                        return 1043.0/229.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.879088997841f ) {
                                    return 1274.0/292.4;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 42206.5f ) {
                                        return 1625.0/192.9;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.04188370705f ) {
                                            return 1952.0/205.1;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.glue_rel_long <= 1.13352560997f ) {
                                                    return 1175.0/142.1;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        if ( cl->stats.glue_rel_queue <= 1.378221035f ) {
                                                            return 1709.0/85.3;
                                                        } else {
                                                            return 1374.0/42.6;
                                                        }
                                                    } else {
                                                        return 1598.0/162.4;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 151722.0f ) {
                                                    return 1041.0/44.7;
                                                } else {
                                                    return 1148.0/18.3;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_3(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_last_touched_diff <= 33346.5f ) {
        if ( rdb0_last_touched_diff <= 10015.5f ) {
            if ( cl->stats.size_rel <= 0.539313018322f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            if ( cl->size() <= 7.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                    return 425.0/2915.5;
                                                } else {
                                                    return 396.0/2472.9;
                                                }
                                            } else {
                                                return 518.0/2690.1;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                                return 187.0/1845.5;
                                            } else {
                                                return 326.0/2263.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            if ( rdb0_last_touched_diff <= 2462.0f ) {
                                                return 231.0/1695.3;
                                            } else {
                                                return 286.0/1612.0;
                                            }
                                        } else {
                                            return 493.0/2280.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.318702399731f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 20076.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0661240369081f ) {
                                                return 233.0/2627.2;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 11928.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 5643.0f ) {
                                                        return 122.0/2048.6;
                                                    } else {
                                                        return 155.0/2127.7;
                                                    }
                                                } else {
                                                    return 126.0/2470.9;
                                                }
                                            }
                                        } else {
                                            return 221.0/1951.1;
                                        }
                                    } else {
                                        return 187.0/3626.1;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0882374793291f ) {
                                                return 312.0/2450.6;
                                            } else {
                                                return 194.0/2012.0;
                                            }
                                        } else {
                                            return 318.0/1975.5;
                                        }
                                    } else {
                                        return 171.0/2026.2;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0970093011856f ) {
                                            return 192.0/1788.7;
                                        } else {
                                            return 255.0/3573.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4228.5f ) {
                                            if ( cl->stats.size_rel <= 0.268765866756f ) {
                                                if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                    return 138.0/2103.4;
                                                } else {
                                                    return 79.0/2259.7;
                                                }
                                            } else {
                                                return 47.0/2178.5;
                                            }
                                        } else {
                                            return 179.0/3082.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( rdb0_last_touched_diff <= 4284.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                            return 304.0/2008.0;
                                        } else {
                                            return 384.0/1945.0;
                                        }
                                    } else {
                                        return 310.0/2696.2;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0928819477558f ) {
                                        return 279.0/1782.6;
                                    } else {
                                        return 617.0/2255.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0492996498942f ) {
                                        return 209.0/1938.9;
                                    } else {
                                        return 296.0/3301.2;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.116241306067f ) {
                                        return 122.0/2117.6;
                                    } else {
                                        return 138.0/3191.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.599830627441f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 333.0/2109.5;
                                } else {
                                    return 183.0/1821.2;
                                }
                            } else {
                                return 459.0/2261.7;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                return 284.0/1549.1;
                            } else {
                                return 420.0/1248.6;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0939934551716f ) {
                                    if ( cl->stats.dump_number <= 21.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 234.0/2381.5;
                                        } else {
                                            return 210.0/2016.1;
                                        }
                                    } else {
                                        return 141.0/2211.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.18507668376f ) {
                                        return 137.0/2109.5;
                                    } else {
                                        return 152.0/3008.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.425292342901f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1633.0f ) {
                                        return 221.0/2328.7;
                                    } else {
                                        return 386.0/2625.2;
                                    }
                                } else {
                                    return 171.0/1823.2;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0570963919163f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1457.5f ) {
                                    return 108.0/1869.9;
                                } else {
                                    return 161.0/1997.8;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.246767312288f ) {
                                    if ( cl->size() <= 5.5f ) {
                                        return 74.0/2349.0;
                                    } else {
                                        return 94.0/1995.8;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 138.0/2832.2;
                                    } else {
                                        return 235.0/3149.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.0536816418171f ) {
                            if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    return 163.0/3601.7;
                                } else {
                                    return 149.0/2048.6;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 2329.0f ) {
                                    if ( rdb0_last_touched_diff <= 3019.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 114.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.00794737040997f ) {
                                                    return 22.0/2513.5;
                                                } else {
                                                    return 57.0/3242.4;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 1005.5f ) {
                                                    if ( rdb0_last_touched_diff <= 796.5f ) {
                                                        return 77.0/4127.6;
                                                    } else {
                                                        return 74.0/2093.2;
                                                    }
                                                } else {
                                                    return 34.0/2129.8;
                                                }
                                            }
                                        } else {
                                            return 106.0/2296.3;
                                        }
                                    } else {
                                        return 124.0/2121.6;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0117851672694f ) {
                                        return 73.0/2016.1;
                                    } else {
                                        return 100.0/2152.1;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.434496581554f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                            return 69.0/3473.8;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.294261991978f ) {
                                                return 39.0/2507.4;
                                            } else {
                                                return 32.0/4062.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 4.5f ) {
                                            return 62.0/2369.3;
                                        } else {
                                            return 66.0/3315.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0769255459309f ) {
                                        return 146.0/3175.4;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 3409.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 1500.0f ) {
                                                    return 70.0/3319.5;
                                                } else {
                                                    return 118.0/2980.5;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.324696838856f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.105589792132f ) {
                                                        return 66.0/2040.4;
                                                    } else {
                                                        return 54.0/2840.4;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.199735164642f ) {
                                                        if ( cl->stats.dump_number <= 8.5f ) {
                                                            return 45.0/3417.0;
                                                        } else {
                                                            return 90.0/3006.9;
                                                        }
                                                    } else {
                                                        if ( cl->stats.used_for_uip_creation <= 30.5f ) {
                                                            return 38.0/3417.0;
                                                        } else {
                                                            if ( cl->stats.glue <= 7.5f ) {
                                                                return 13.0/2139.9;
                                                            } else {
                                                                return 7.0/2152.1;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.492784291506f ) {
                                                return 111.0/2426.2;
                                            } else {
                                                return 142.0/2133.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 98.0/2079.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                    if ( rdb0_last_touched_diff <= 4134.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.27102380991f ) {
                            if ( rdb0_last_touched_diff <= 898.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    return 245.0/2744.9;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.980000019073f ) {
                                        if ( cl->size() <= 36.5f ) {
                                            return 35.0/2655.6;
                                        } else {
                                            return 81.0/3051.5;
                                        }
                                    } else {
                                        return 91.0/2178.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 112.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 209.0/2937.8;
                                        } else {
                                            return 114.0/2198.8;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            return 284.0/1571.4;
                                        } else {
                                            return 224.0/3207.8;
                                        }
                                    }
                                } else {
                                    return 221.0/1999.8;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    return 405.0/2077.0;
                                } else {
                                    return 261.0/1930.8;
                                }
                            } else {
                                if ( cl->size() <= 27.5f ) {
                                    return 123.0/1926.7;
                                } else {
                                    return 130.0/3453.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.322278916836f ) {
                            if ( rdb0_last_touched_diff <= 7832.5f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    return 251.0/2184.6;
                                } else {
                                    return 276.0/1920.6;
                                }
                            } else {
                                return 371.0/1585.7;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                return 408.0/1268.9;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.62376511097f ) {
                                    return 269.0/1654.7;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 771.0/2600.8;
                                    } else {
                                        return 214.0/1930.8;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.700888693333f ) {
                        return 261.0/1591.7;
                    } else {
                        return 638.0/2273.9;
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 43.5f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 19825.0f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.size_rel <= 0.0868457108736f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0389876291156f ) {
                                    return 452.0/1329.8;
                                } else {
                                    return 483.0/2493.2;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0304587893188f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0929765254259f ) {
                                            return 742.0/2257.7;
                                        } else {
                                            return 491.0/1128.8;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0825900360942f ) {
                                                return 369.0/1425.3;
                                            } else {
                                                return 403.0/2022.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.494054615498f ) {
                                                return 442.0/1841.5;
                                            } else {
                                                return 381.0/1331.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0693986713886f ) {
                                        return 547.0/970.5;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.551325798035f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 648.0/2139.9;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.413162380457f ) {
                                                        return 558.0/1386.7;
                                                    } else {
                                                        return 424.0/1317.7;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.725561499596f ) {
                                                    if ( rdb0_last_touched_diff <= 20699.0f ) {
                                                        return 471.0/1145.1;
                                                    } else {
                                                        return 540.0/1405.0;
                                                    }
                                                } else {
                                                    return 847.0/1794.8;
                                                }
                                            }
                                        } else {
                                            return 509.0/1869.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.70675355196f ) {
                                        if ( cl->stats.glue_rel_long <= 0.563752949238f ) {
                                            return 857.0/1760.3;
                                        } else {
                                            return 405.0/1234.4;
                                        }
                                    } else {
                                        return 831.0/1610.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.62274324894f ) {
                                            return 661.0/1715.6;
                                        } else {
                                            return 830.0/1350.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.8467258811f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0889515727758f ) {
                                                return 586.0/919.7;
                                            } else {
                                                return 642.0/1210.0;
                                            }
                                        } else {
                                            return 659.0/730.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0795167535543f ) {
                                    if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                        return 1014.0/2016.1;
                                    } else {
                                        return 706.0/1173.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 58.5f ) {
                                            return 951.0/1587.7;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.874920845032f ) {
                                                return 817.0/1289.2;
                                            } else {
                                                return 727.0/659.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.37968754768f ) {
                                            return 741.0/700.4;
                                        } else {
                                            return 1138.0/720.8;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 7.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.20200112462f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        return 547.0/1378.6;
                                    } else {
                                        return 671.0/1005.0;
                                    }
                                } else {
                                    return 724.0/1063.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                        return 751.0/2267.8;
                                    } else {
                                        return 748.0/1737.9;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            return 774.0/2263.8;
                                        } else {
                                            return 574.0/1094.3;
                                        }
                                    } else {
                                        return 795.0/1597.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 967.0/1061.8;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                        return 804.0/700.4;
                                    } else {
                                        return 1135.0/473.1;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 35918.0f ) {
                                    return 956.0/1845.5;
                                } else {
                                    return 1207.0/1756.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 3.10005664825f ) {
                        if ( rdb0_last_touched_diff <= 16283.5f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 490.0/2491.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 13676.5f ) {
                                        if ( rdb0_last_touched_diff <= 11291.5f ) {
                                            return 232.0/1746.0;
                                        } else {
                                            return 263.0/2600.8;
                                        }
                                    } else {
                                        return 270.0/1768.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.543822586536f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0770297348499f ) {
                                        if ( cl->stats.size_rel <= 0.191090792418f ) {
                                            return 360.0/1904.4;
                                        } else {
                                            return 377.0/1474.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3081.5f ) {
                                            return 375.0/2686.1;
                                        } else {
                                            return 323.0/1733.9;
                                        }
                                    }
                                } else {
                                    return 605.0/2310.5;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 482.0/1439.5;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 360.0/1662.8;
                                } else {
                                    return 318.0/1557.2;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 13464.0f ) {
                            return 373.0/1362.3;
                        } else {
                            return 419.0/1195.8;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.943920314312f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 409.0/1293.3;
                            } else {
                                if ( cl->size() <= 17.5f ) {
                                    return 514.0/1082.1;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.556486129761f ) {
                                        return 733.0/1153.2;
                                    } else {
                                        return 627.0/747.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 13.5f ) {
                                return 877.0/783.7;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.13754582405f ) {
                                    return 854.0/485.2;
                                } else {
                                    return 977.0/280.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 210.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.842999577522f ) {
                                if ( cl->stats.size_rel <= 0.436658680439f ) {
                                    return 830.0/1683.1;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 2.86079859734f ) {
                                        return 658.0/822.3;
                                    } else {
                                        return 817.0/824.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.12003588676f ) {
                                    if ( cl->stats.size_rel <= 0.548466324806f ) {
                                        return 721.0/651.7;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.877190470695f ) {
                                            return 1089.0/607.1;
                                        } else {
                                            return 870.0/389.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 14.5f ) {
                                        return 1154.0/519.8;
                                    } else {
                                        return 1362.0/337.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 1489.0/487.3;
                                } else {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        return 1667.0/657.8;
                                    } else {
                                        if ( cl->size() <= 129.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 579.5f ) {
                                                if ( cl->stats.glue <= 23.5f ) {
                                                    return 1457.0/186.8;
                                                } else {
                                                    return 1077.0/239.6;
                                                }
                                            } else {
                                                return 1025.0/85.3;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                                return 1157.0/77.2;
                                            } else {
                                                return 1162.0/123.8;
                                            }
                                        }
                                    }
                                }
                            } else {
                                return 1145.0/1189.7;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 1.03394913673f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 2821.5f ) {
                            return 685.0/2257.7;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.770263195038f ) {
                                return 565.0/2054.6;
                            } else {
                                return 736.0/1273.0;
                            }
                        }
                    } else {
                        return 1137.0/1220.2;
                    }
                }
            }
        }
    } else {
        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.size_rel <= 0.614662587643f ) {
                if ( cl->size() <= 9.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 44464.0f ) {
                                if ( rdb0_last_touched_diff <= 46601.0f ) {
                                    return 564.0/1463.8;
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        return 535.0/1319.7;
                                    } else {
                                        return 613.0/1151.2;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 6.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0531296916306f ) {
                                        return 951.0/1439.5;
                                    } else {
                                        return 1258.0/2391.7;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.312250137329f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0695144459605f ) {
                                            return 1033.0/1668.9;
                                        } else {
                                            return 837.0/946.1;
                                        }
                                    } else {
                                        return 732.0/663.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 4.5f ) {
                                return 911.0/1122.7;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.61868429184f ) {
                                    return 693.0/716.7;
                                } else {
                                    return 724.0/657.8;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                return 649.0/978.6;
                            } else {
                                if ( rdb0_last_touched_diff <= 226489.0f ) {
                                    return 1112.0/1380.6;
                                } else {
                                    return 921.0/625.3;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 154671.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.52501344681f ) {
                                    if ( cl->stats.size_rel <= 0.140596225858f ) {
                                        return 884.0/1155.2;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.156274735928f ) {
                                            return 1170.0/931.9;
                                        } else {
                                            return 1287.0/1390.7;
                                        }
                                    }
                                } else {
                                    return 900.0/596.9;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        return 737.0/609.1;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.583747982979f ) {
                                            return 974.0/668.0;
                                        } else {
                                            return 834.0/400.0;
                                        }
                                    }
                                } else {
                                    return 1627.0/820.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 147.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.160845845938f ) {
                                    return 755.0/1230.4;
                                } else {
                                    return 702.0/879.1;
                                }
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.245206773281f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.162957385182f ) {
                                            return 1345.0/1228.3;
                                        } else {
                                            return 784.0/570.5;
                                        }
                                    } else {
                                        return 1283.0/1291.3;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.724200606346f ) {
                                        return 1347.0/986.7;
                                    } else {
                                        return 870.0/406.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.960030674934f ) {
                                return 899.0/448.7;
                            } else {
                                return 1187.0/209.1;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 128.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.559358656406f ) {
                                if ( cl->stats.dump_number <= 16.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 67124.5f ) {
                                            return 628.0/862.9;
                                        } else {
                                            return 760.0/481.2;
                                        }
                                    } else {
                                        return 800.0/850.7;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.294667184353f ) {
                                        return 1535.0/1027.3;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 226626.5f ) {
                                            return 1469.0/901.4;
                                        } else {
                                            return 1332.0/349.2;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.142841607332f ) {
                                            return 917.0/428.4;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 165940.0f ) {
                                                return 1176.0/1019.2;
                                            } else {
                                                return 849.0/351.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.778900504112f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                return 1321.0/688.3;
                                            } else {
                                                return 1342.0/546.1;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 71623.0f ) {
                                                return 969.0/467.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.275757372379f ) {
                                                    return 1797.0/393.9;
                                                } else {
                                                    return 958.0/318.8;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 153984.0f ) {
                                        return 1047.0/521.8;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.802422821522f ) {
                                            return 970.0/239.6;
                                        } else {
                                            return 951.0/180.7;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.599959254265f ) {
                                return 982.0/562.4;
                            } else {
                                if ( cl->stats.size_rel <= 0.395525157452f ) {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        return 1103.0/328.9;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 393.5f ) {
                                            return 964.0/219.3;
                                        } else {
                                            return 1227.0/164.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        return 1059.0/261.9;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.09957194328f ) {
                                            return 1418.0/245.7;
                                        } else {
                                            return 1572.0/158.4;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 105350.5f ) {
                    if ( cl->stats.glue <= 11.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.865290522575f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.dump_number <= 9.5f ) {
                                    if ( rdb0_last_touched_diff <= 62131.0f ) {
                                        return 911.0/745.1;
                                    } else {
                                        return 875.0/462.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.183351069689f ) {
                                        return 733.0/893.3;
                                    } else {
                                        return 948.0/810.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 8.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.350523084402f ) {
                                        return 877.0/601.0;
                                    } else {
                                        return 832.0/464.9;
                                    }
                                } else {
                                    return 1094.0/442.6;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.01234567165f ) {
                                    return 942.0/469.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 1269.0/475.1;
                                    } else {
                                        return 1291.0/241.6;
                                    }
                                }
                            } else {
                                return 897.0/605.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.00373101234f ) {
                            if ( cl->stats.glue_rel_queue <= 0.839043498039f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 80.5f ) {
                                    return 729.0/665.9;
                                } else {
                                    return 1674.0/763.4;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    if ( rdb0_last_touched_diff <= 58958.5f ) {
                                        return 880.0/280.2;
                                    } else {
                                        return 1381.0/300.5;
                                    }
                                } else {
                                    return 1060.0/578.6;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 262.5f ) {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.651207745075f ) {
                                        if ( rdb0_last_touched_diff <= 63369.0f ) {
                                            return 1046.0/290.3;
                                        } else {
                                            return 1649.0/335.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 6.5f ) {
                                            return 1778.0/259.9;
                                        } else {
                                            return 1844.0/160.4;
                                        }
                                    }
                                } else {
                                    return 799.0/598.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.20641815662f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 1124.0/205.1;
                                    } else {
                                        return 1710.0/194.9;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 22.5f ) {
                                        return 1033.0/115.7;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 703.5f ) {
                                            return 1187.0/103.5;
                                        } else {
                                            return 1145.0/44.7;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.935611724854f ) {
                        if ( rdb0_last_touched_diff <= 189326.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.771889328957f ) {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.649069786072f ) {
                                        return 876.0/389.8;
                                    } else {
                                        return 848.0/473.1;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.873352527618f ) {
                                        return 1199.0/497.4;
                                    } else {
                                        return 1152.0/294.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 144966.0f ) {
                                    if ( cl->stats.dump_number <= 13.5f ) {
                                        return 1269.0/243.6;
                                    } else {
                                        return 1034.0/442.6;
                                    }
                                } else {
                                    return 1160.0/237.5;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.85396361351f ) {
                                if ( rdb0_last_touched_diff <= 270079.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.710160851479f ) {
                                            return 1127.0/322.8;
                                        } else {
                                            return 925.0/339.1;
                                        }
                                    } else {
                                        return 951.0/205.1;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.318301200867f ) {
                                        if ( cl->size() <= 22.5f ) {
                                            return 923.0/314.7;
                                        } else {
                                            return 1109.0/227.4;
                                        }
                                    } else {
                                        if ( cl->size() <= 20.5f ) {
                                            return 1310.0/255.8;
                                        } else {
                                            return 1538.0/211.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 35.5f ) {
                                    return 1547.0/180.7;
                                } else {
                                    return 1050.0/237.5;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 371.5f ) {
                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                return 1012.0/288.3;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.dump_number <= 46.5f ) {
                                        if ( cl->stats.glue <= 14.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.948320448399f ) {
                                                return 1000.0/255.8;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.606046795845f ) {
                                                        return 1356.0/233.5;
                                                    } else {
                                                        return 1621.0/146.2;
                                                    }
                                                } else {
                                                    return 1630.0/391.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 113093.0f ) {
                                                return 1002.0/182.7;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.09520220757f ) {
                                                    return 1623.0/241.6;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.43331944942f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                            return 1245.0/73.1;
                                                        } else {
                                                            return 1287.0/203.0;
                                                        }
                                                    } else {
                                                        return 1554.0/115.7;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.21813511848f ) {
                                            return 1270.0/385.8;
                                        } else {
                                            return 945.0/164.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 243003.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 32.5f ) {
                                            return 948.0/164.5;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 163497.0f ) {
                                                return 1576.0/213.2;
                                            } else {
                                                return 1581.0/123.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.12982058525f ) {
                                            return 1019.0/75.1;
                                        } else {
                                            return 1373.0/24.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 1.55465531349f ) {
                                if ( cl->size() <= 58.5f ) {
                                    return 1386.0/107.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.16245698929f ) {
                                        return 1181.0/162.4;
                                    } else {
                                        return 1034.0/93.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 2.62997603416f ) {
                                    if ( cl->stats.size_rel <= 1.92411017418f ) {
                                        return 1027.0/34.5;
                                    } else {
                                        return 1136.0/101.5;
                                    }
                                } else {
                                    return 1268.0/28.4;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 3635.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.68104660511f ) {
                    if ( cl->stats.size_rel <= 0.336366117001f ) {
                        if ( rdb0_last_touched_diff <= 1751.0f ) {
                            return 319.0/3211.9;
                        } else {
                            return 290.0/1409.0;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                            return 260.0/1987.6;
                        } else {
                            return 346.0/1896.3;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                        return 366.0/2079.0;
                    } else {
                        return 566.0/2428.2;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.812228560448f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            return 548.0/1480.1;
                        } else {
                            return 832.0/1707.5;
                        }
                    } else {
                        return 504.0/2328.7;
                    }
                } else {
                    return 821.0/1459.8;
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_4(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( cl->stats.num_overlap_literals <= 50.5f ) {
                if ( rdb0_last_touched_diff <= 9998.5f ) {
                    if ( cl->stats.glue_rel_long <= 0.555339097977f ) {
                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                            if ( rdb0_last_touched_diff <= 3113.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0622476413846f ) {
                                    return 213.0/1823.2;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        return 328.0/3122.6;
                                    } else {
                                        return 149.0/3563.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0293111465871f ) {
                                    return 549.0/2594.7;
                                } else {
                                    return 455.0/2621.1;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 28003.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16170.5f ) {
                                            return 378.0/2479.0;
                                        } else {
                                            return 383.0/1987.6;
                                        }
                                    } else {
                                        return 348.0/1601.9;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        return 618.0/1737.9;
                                    } else {
                                        return 504.0/2010.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.155340254307f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 20873.5f ) {
                                        return 160.0/1815.1;
                                    } else {
                                        return 205.0/1646.6;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.165270239115f ) {
                                        return 92.0/1926.7;
                                    } else {
                                        return 183.0/2202.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( rdb0_last_touched_diff <= 2780.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 38696.0f ) {
                                    return 520.0/2822.1;
                                } else {
                                    return 405.0/1291.3;
                                }
                            } else {
                                if ( cl->size() <= 15.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.696486592293f ) {
                                        return 537.0/1423.2;
                                    } else {
                                        return 508.0/1918.6;
                                    }
                                } else {
                                    return 866.0/1906.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0740765854716f ) {
                                    return 233.0/2095.3;
                                } else {
                                    return 203.0/2442.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.61839735508f ) {
                                    return 189.0/1624.2;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.136762410402f ) {
                                        return 307.0/1670.9;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                            return 354.0/2072.9;
                                        } else {
                                            return 229.0/2655.6;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.269711494446f ) {
                        if ( rdb0_last_touched_diff <= 48448.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0957607626915f ) {
                                    if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                        return 573.0/2273.9;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0391473025084f ) {
                                            return 630.0/2223.2;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.105472825468f ) {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 443.0/1256.7;
                                                } else {
                                                    return 541.0/1019.2;
                                                }
                                            } else {
                                                return 527.0/1545.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( rdb0_last_touched_diff <= 28038.0f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.14851886034f ) {
                                                return 367.0/1565.3;
                                            } else {
                                                return 460.0/1496.3;
                                            }
                                        } else {
                                            return 878.0/2206.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16544.0f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.154793083668f ) {
                                                return 502.0/1171.5;
                                            } else {
                                                return 508.0/1043.6;
                                            }
                                        } else {
                                            if ( cl->size() <= 15.5f ) {
                                                return 628.0/1065.9;
                                            } else {
                                                return 957.0/1232.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0521236658096f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 22985.5f ) {
                                            return 468.0/1090.3;
                                        } else {
                                            return 584.0/1053.7;
                                        }
                                    } else {
                                        return 464.0/1407.0;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.157536640763f ) {
                                        return 896.0/1348.1;
                                    } else {
                                        return 734.0/1506.5;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.541614055634f ) {
                                    return 882.0/1792.7;
                                } else {
                                    return 1111.0/1465.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0727669000626f ) {
                                    return 1129.0/1628.3;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.771397352219f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 56740.5f ) {
                                                return 664.0/976.6;
                                            } else {
                                                return 678.0/728.9;
                                            }
                                        } else {
                                            return 1135.0/856.8;
                                        }
                                    } else {
                                        return 883.0/538.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.88171505928f ) {
                            if ( cl->stats.size_rel <= 0.666578292847f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 32619.0f ) {
                                    return 853.0/2213.0;
                                } else {
                                    return 678.0/866.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    return 1178.0/1161.3;
                                } else {
                                    return 846.0/1076.1;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 10.5f ) {
                                return 630.0/812.1;
                            } else {
                                if ( cl->stats.size_rel <= 1.06020760536f ) {
                                    return 706.0/615.2;
                                } else {
                                    return 983.0/428.4;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 10.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 22113.0f ) {
                        if ( cl->stats.glue_rel_queue <= 0.73868894577f ) {
                            if ( cl->stats.glue_rel_long <= 0.517291903496f ) {
                                return 346.0/1439.5;
                            } else {
                                return 386.0/1264.9;
                            }
                        } else {
                            return 937.0/2010.0;
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.size_rel <= 0.482685476542f ) {
                                    return 525.0/1043.6;
                                } else {
                                    return 689.0/668.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.788385987282f ) {
                                    return 966.0/659.8;
                                } else {
                                    return 811.0/751.2;
                                }
                            }
                        } else {
                            return 310.0/1737.9;
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.01107394695f ) {
                        return 743.0/1472.0;
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.939065337181f ) {
                            if ( rdb0_last_touched_diff <= 13048.5f ) {
                                return 354.0/1606.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.778811335564f ) {
                                    return 884.0/944.1;
                                } else {
                                    return 1278.0/777.6;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 40441.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.775041401386f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.650903820992f ) {
                                        return 793.0/820.2;
                                    } else {
                                        return 793.0/586.8;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 19.996799469f ) {
                                        return 1079.0/655.8;
                                    } else {
                                        return 1023.0/337.0;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1187.0/663.9;
                                } else {
                                    if ( cl->stats.glue <= 24.5f ) {
                                        return 1217.0/288.3;
                                    } else {
                                        return 1014.0/97.5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.75733590126f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 45724.0f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.422349393368f ) {
                            if ( cl->stats.size_rel <= 0.116921283305f ) {
                                return 624.0/1953.1;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.148108541965f ) {
                                    return 535.0/1317.7;
                                } else {
                                    return 828.0/1819.1;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.049732670188f ) {
                                return 805.0/1279.1;
                            } else {
                                if ( rdb0_last_touched_diff <= 27561.5f ) {
                                    return 429.0/1244.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.564652919769f ) {
                                        return 592.0/1179.6;
                                    } else {
                                        return 825.0/1547.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0214592963457f ) {
                            if ( cl->stats.glue_rel_long <= 0.461115449667f ) {
                                return 845.0/1849.6;
                            } else {
                                if ( cl->size() <= 20.5f ) {
                                    return 710.0/895.4;
                                } else {
                                    return 676.0/1031.4;
                                }
                            }
                        } else {
                            if ( cl->size() <= 70.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    if ( cl->stats.dump_number <= 5.5f ) {
                                        return 785.0/836.5;
                                    } else {
                                        return 601.0/820.2;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                        return 708.0/905.5;
                                    } else {
                                        return 1247.0/929.9;
                                    }
                                }
                            } else {
                                return 826.0/458.8;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 152440.0f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 100858.5f ) {
                                if ( cl->stats.size_rel <= 0.387140184641f ) {
                                    if ( rdb0_last_touched_diff <= 89607.5f ) {
                                        if ( cl->stats.dump_number <= 24.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.337996006012f ) {
                                                return 700.0/777.6;
                                            } else {
                                                return 1179.0/1612.0;
                                            }
                                        } else {
                                            return 669.0/1112.6;
                                        }
                                    } else {
                                        return 1071.0/1675.0;
                                    }
                                } else {
                                    return 714.0/757.3;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.193307876587f ) {
                                        return 931.0/1177.6;
                                    } else {
                                        return 665.0/702.5;
                                    }
                                } else {
                                    return 957.0/816.2;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.01513671875f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.103496238589f ) {
                                    return 1033.0/1027.3;
                                } else {
                                    if ( cl->stats.size_rel <= 0.418517440557f ) {
                                        return 717.0/824.3;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1253.0/901.4;
                                        } else {
                                            return 1249.0/696.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.794224262238f ) {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 848.0/503.5;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                if ( cl->stats.dump_number <= 10.5f ) {
                                                    return 933.0/385.8;
                                                } else {
                                                    return 961.0/442.6;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.591464698315f ) {
                                                    return 1140.0/552.2;
                                                } else {
                                                    return 1376.0/430.4;
                                                }
                                            }
                                        }
                                    } else {
                                        return 958.0/192.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.488108217716f ) {
                                        return 774.0/635.5;
                                    } else {
                                        if ( cl->size() <= 18.5f ) {
                                            return 1112.0/810.1;
                                        } else {
                                            return 942.0/440.6;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->size() <= 5.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0724891275167f ) {
                                    return 1257.0/974.5;
                                } else {
                                    return 667.0/808.1;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.474417924881f ) {
                                    return 1473.0/862.9;
                                } else {
                                    return 1246.0/848.7;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0970883220434f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 264253.0f ) {
                                        return 1233.0/694.4;
                                    } else {
                                        return 1386.0/485.2;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.396380037069f ) {
                                        return 1350.0/582.7;
                                    } else {
                                        return 1175.0/286.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 329263.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.630194544792f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.310363471508f ) {
                                            return 1455.0/428.4;
                                        } else {
                                            return 1454.0/641.6;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.739273905754f ) {
                                            return 1775.0/432.5;
                                        } else {
                                            return 1291.0/446.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.39199078083f ) {
                                        if ( cl->stats.dump_number <= 58.5f ) {
                                            return 1202.0/190.8;
                                        } else {
                                            return 1047.0/353.3;
                                        }
                                    } else {
                                        return 1041.0/158.4;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 9.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 78918.0f ) {
                        if ( cl->stats.num_overlap_literals <= 13.5f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 635.0/1104.5;
                            } else {
                                return 1297.0/1431.4;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->size() <= 13.5f ) {
                                    return 1022.0/1287.2;
                                } else {
                                    return 1128.0/755.3;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.428097695112f ) {
                                    return 925.0/623.3;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.484186649323f ) {
                                        return 928.0/493.4;
                                    } else {
                                        return 1141.0/428.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 1119.0/919.7;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 1229.0/657.8;
                                } else {
                                    return 1294.0/517.7;
                                }
                            }
                        } else {
                            if ( cl->size() <= 18.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    return 1323.0/418.2;
                                } else {
                                    return 1409.0/326.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 1035.0/304.5;
                                } else {
                                    return 1897.0/324.8;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.295317351818f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                            if ( rdb0_last_touched_diff <= 41967.5f ) {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    return 726.0/726.8;
                                } else {
                                    return 807.0/460.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.03935003281f ) {
                                    return 1016.0/499.4;
                                } else {
                                    return 971.0/290.3;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.849593877792f ) {
                                if ( cl->stats.dump_number <= 22.5f ) {
                                    return 1597.0/844.6;
                                } else {
                                    return 1167.0/308.6;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 1.17850613594f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.11238206923f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 112.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.67708349228f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                                    return 961.0/371.5;
                                                } else {
                                                    return 888.0/692.3;
                                                }
                                            } else {
                                                return 1063.0/225.4;
                                            }
                                        } else {
                                            return 1071.0/249.7;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.6724203825f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 102086.5f ) {
                                                return 949.0/385.8;
                                            } else {
                                                return 1163.0/239.6;
                                            }
                                        } else {
                                            if ( cl->size() <= 46.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                                    return 1249.0/268.0;
                                                } else {
                                                    return 976.0/140.1;
                                                }
                                            } else {
                                                return 933.0/268.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 47.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0546875f ) {
                                            return 1083.0/306.6;
                                        } else {
                                            return 1733.0/282.2;
                                        }
                                    } else {
                                        return 1058.0/62.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 34463.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.16266262531f ) {
                                    if ( cl->stats.glue_rel_long <= 0.9683842659f ) {
                                        return 870.0/328.9;
                                    } else {
                                        return 1735.0/412.1;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 17.5f ) {
                                        return 1103.0/223.3;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.38260865211f ) {
                                            return 952.0/121.8;
                                        } else {
                                            return 1077.0/75.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 15.4138774872f ) {
                                    return 1029.0/601.0;
                                } else {
                                    return 1004.0/298.5;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 15.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 114743.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.95343542099f ) {
                                        if ( cl->size() <= 17.5f ) {
                                            return 1230.0/493.4;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.974043607712f ) {
                                                return 926.0/312.7;
                                            } else {
                                                if ( cl->stats.glue <= 12.5f ) {
                                                    return 998.0/227.4;
                                                } else {
                                                    return 1196.0/196.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 1.63311696053f ) {
                                                return 929.0/186.8;
                                            } else {
                                                return 959.0/233.5;
                                            }
                                        } else {
                                            return 1382.0/201.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.05059242249f ) {
                                        if ( cl->stats.dump_number <= 30.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.923018813133f ) {
                                                return 999.0/231.5;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 141.5f ) {
                                                    return 1379.0/280.2;
                                                } else {
                                                    return 1464.0/154.3;
                                                }
                                            }
                                        } else {
                                            return 1756.0/471.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.58588314056f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 1356.0/132.0;
                                                } else {
                                                    return 1155.0/308.6;
                                                }
                                            } else {
                                                return 1255.0/101.5;
                                            }
                                        } else {
                                            return 1657.0/142.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 2.43905162811f ) {
                                    if ( cl->stats.dump_number <= 48.5f ) {
                                        if ( rdb0_last_touched_diff <= 100623.0f ) {
                                            if ( cl->stats.dump_number <= 8.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 185.5f ) {
                                                    if ( rdb0_last_touched_diff <= 63667.5f ) {
                                                        return 1073.0/170.5;
                                                    } else {
                                                        return 1200.0/125.9;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.60349392891f ) {
                                                            return 1043.0/38.6;
                                                        } else {
                                                            return 991.0/18.3;
                                                        }
                                                    } else {
                                                        return 1527.0/194.9;
                                                    }
                                                }
                                            } else {
                                                return 1488.0/446.7;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 251.5f ) {
                                                    if ( cl->stats.glue <= 19.5f ) {
                                                        return 1419.0/178.7;
                                                    } else {
                                                        return 1032.0/60.9;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.892519712448f ) {
                                                        return 1558.0/101.5;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 192583.0f ) {
                                                            return 1270.0/77.2;
                                                        } else {
                                                            return 1031.0/24.4;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.12954306602f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.703941822052f ) {
                                                        return 982.0/138.1;
                                                    } else {
                                                        return 1026.0/207.1;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue <= 22.5f ) {
                                                        return 1822.0/199.0;
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 168644.0f ) {
                                                            return 1361.0/129.9;
                                                        } else {
                                                            return 1372.0/52.8;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 1353.0/288.3;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 963.5f ) {
                                        return 1799.0/105.6;
                                    } else {
                                        return 1042.0/8.1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.num_total_lits_antecedents <= 166.5f ) {
            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.857527852058f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0374922417104f ) {
                            if ( cl->stats.size_rel <= 0.122821763158f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 522.0/2895.2;
                                } else {
                                    return 449.0/1800.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 13694.5f ) {
                                    return 554.0/2533.8;
                                } else {
                                    return 822.0/2342.9;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.115911990404f ) {
                                    return 467.0/1790.7;
                                } else {
                                    return 410.0/1433.4;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 5188.0f ) {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            return 226.0/1772.4;
                                        } else {
                                            return 248.0/1585.7;
                                        }
                                    } else {
                                        return 337.0/1831.3;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.47829866409f ) {
                                        if ( rdb0_last_touched_diff <= 12346.0f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1196.5f ) {
                                                return 346.0/1545.0;
                                            } else {
                                                return 271.0/1618.1;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.164036080241f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 508.0/1938.9;
                                                } else {
                                                    return 421.0/1354.2;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 15594.0f ) {
                                                    return 366.0/1400.9;
                                                } else {
                                                    return 357.0/1748.1;
                                                }
                                            }
                                        }
                                    } else {
                                        return 278.0/1691.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.146274417639f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 560.0/1435.4;
                                } else {
                                    return 649.0/1309.5;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.613287985325f ) {
                                    return 864.0/1768.4;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                        return 744.0/978.6;
                                    } else {
                                        return 601.0/970.5;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 4472.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1185.5f ) {
                                    return 306.0/1666.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                        return 411.0/2077.0;
                                    } else {
                                        return 385.0/1372.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                    return 550.0/1463.8;
                                } else {
                                    return 399.0/1551.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 1.00888895988f ) {
                        if ( cl->stats.size_rel <= 0.81156027317f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.191135376692f ) {
                                    return 526.0/1057.8;
                                } else {
                                    return 599.0/966.4;
                                }
                            } else {
                                return 438.0/1650.6;
                            }
                        } else {
                            return 857.0/978.6;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 83.5f ) {
                                return 760.0/627.4;
                            } else {
                                return 913.0/357.3;
                            }
                        } else {
                            return 923.0/2280.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.288456559181f ) {
                                        return 364.0/2393.7;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                return 202.0/1794.8;
                                            } else {
                                                return 245.0/1731.8;
                                            }
                                        } else {
                                            if ( cl->size() <= 5.5f ) {
                                                return 200.0/2194.7;
                                            } else {
                                                return 297.0/3077.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 292.0/1892.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.60364317894f ) {
                                    return 446.0/2939.9;
                                } else {
                                    return 571.0/2893.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0525907948613f ) {
                                    if ( cl->stats.glue <= 3.5f ) {
                                        return 149.0/2509.4;
                                    } else {
                                        return 229.0/2474.9;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.443503677845f ) {
                                                return 174.0/2858.6;
                                            } else {
                                                return 136.0/1831.3;
                                            }
                                        } else {
                                            return 155.0/3307.3;
                                        }
                                    } else {
                                        return 102.0/3327.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.128107696772f ) {
                                        return 235.0/2129.8;
                                    } else {
                                        return 167.0/2176.5;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0444946363568f ) {
                                        return 140.0/2675.9;
                                    } else {
                                        return 126.0/1817.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( rdb0_last_touched_diff <= 4482.0f ) {
                                    return 383.0/2008.0;
                                } else {
                                    return 561.0/1752.1;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                    return 304.0/2154.1;
                                } else {
                                    return 262.0/1616.1;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                return 489.0/2769.3;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.400320529938f ) {
                                    return 161.0/2434.3;
                                } else {
                                    if ( rdb0_last_touched_diff <= 1583.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.672257304192f ) {
                                            return 103.0/1859.7;
                                        } else {
                                            return 138.0/1991.7;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            return 313.0/2318.6;
                                        } else {
                                            return 218.0/2026.2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 3592.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0619575157762f ) {
                                if ( cl->stats.glue_rel_long <= 0.433433890343f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                        return 140.0/2022.2;
                                    } else {
                                        return 182.0/3762.1;
                                    }
                                } else {
                                    return 193.0/2537.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 6111.5f ) {
                                    if ( rdb0_last_touched_diff <= 571.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 32.0/2219.1;
                                        } else {
                                            return 109.0/3319.5;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.447012960911f ) {
                                            if ( cl->stats.size_rel <= 0.27967184782f ) {
                                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                    return 128.0/2464.8;
                                                } else {
                                                    return 88.0/2539.9;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.141363158822f ) {
                                                    return 87.0/2255.6;
                                                } else {
                                                    return 59.0/2066.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                                return 100.0/2194.7;
                                            } else {
                                                return 146.0/2117.6;
                                            }
                                        }
                                    }
                                } else {
                                    return 137.0/1951.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 3233.5f ) {
                                if ( rdb0_last_touched_diff <= 1526.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0309509709477f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.00323664583266f ) {
                                                return 13.0/2188.6;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 8.5f ) {
                                                    return 29.0/2129.8;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 87.5f ) {
                                                        return 87.0/2596.7;
                                                    } else {
                                                        if ( cl->stats.dump_number <= 12.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0247580055147f ) {
                                                                return 52.0/2040.4;
                                                            } else {
                                                                return 36.0/3242.4;
                                                            }
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.109952621162f ) {
                                                                return 33.0/2119.6;
                                                            } else {
                                                                return 66.0/1890.2;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 69.0/2355.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.208736911416f ) {
                                            return 46.0/1985.6;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0781739205122f ) {
                                                return 60.0/2213.0;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 553.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.231416970491f ) {
                                                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                                                return 6.0/3335.8;
                                                            } else {
                                                                return 25.0/2895.2;
                                                            }
                                                        } else {
                                                            return 25.0/2893.2;
                                                        }
                                                    } else {
                                                        return 25.0/2054.6;
                                                    }
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.206203967333f ) {
                                                        return 23.0/2901.3;
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 1158.5f ) {
                                                            return 32.0/3181.5;
                                                        } else {
                                                            if ( cl->stats.rdb1_used_for_uip_creation <= 17.5f ) {
                                                                return 51.0/2089.2;
                                                            } else {
                                                                return 27.0/2066.8;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2515.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 72.0/3508.3;
                                        } else {
                                            return 114.0/3798.7;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                            return 76.0/2773.4;
                                        } else {
                                            return 98.0/1989.7;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                    return 145.0/3057.6;
                                } else {
                                    if ( cl->stats.dump_number <= 16.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.109345600009f ) {
                                            return 54.0/2052.6;
                                        } else {
                                            return 33.0/2054.6;
                                        }
                                    } else {
                                        return 82.0/2139.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.397530853748f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.038668256253f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                        return 169.0/1849.6;
                                    } else {
                                        return 272.0/2131.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 7828.5f ) {
                                        return 249.0/3615.9;
                                    } else {
                                        return 171.0/1697.3;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.316086828709f ) {
                                        return 158.0/1876.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.538358986378f ) {
                                            return 138.0/2673.9;
                                        } else {
                                            return 168.0/2097.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0623905658722f ) {
                                        return 200.0/2994.7;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            return 89.0/2730.7;
                                        } else {
                                            return 135.0/2489.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.586047887802f ) {
                                return 252.0/2878.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 6432.5f ) {
                                    return 172.0/1971.4;
                                } else {
                                    return 230.0/1638.4;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 7.49520730972f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( rdb0_last_touched_diff <= 9376.0f ) {
                        return 371.0/2044.5;
                    } else {
                        if ( cl->size() <= 34.5f ) {
                            return 622.0/895.4;
                        } else {
                            return 974.0/698.4;
                        }
                    }
                } else {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            return 425.0/1453.7;
                        } else {
                            if ( rdb0_last_touched_diff <= 6830.0f ) {
                                return 214.0/2984.5;
                            } else {
                                return 334.0/1553.2;
                            }
                        }
                    } else {
                        return 145.0/3437.3;
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 9992.5f ) {
                    if ( rdb0_last_touched_diff <= 2823.5f ) {
                        return 178.0/2164.3;
                    } else {
                        return 353.0/1498.3;
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 1.02890276909f ) {
                        return 1182.0/1197.9;
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 625.5f ) {
                            return 1513.0/594.9;
                        } else {
                            return 930.0/184.8;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_5(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_last_touched_diff <= 29489.0f ) {
        if ( rdb0_last_touched_diff <= 10015.5f ) {
            if ( rdb0_last_touched_diff <= 3308.5f ) {
                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.num_overlap_literals <= 12.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 5187.0f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0154945664108f ) {
                                return 182.0/1642.5;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 10.5f ) {
                                    return 115.0/1843.5;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 902.5f ) {
                                        return 161.0/1721.7;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0498665384948f ) {
                                            return 178.0/2058.7;
                                        } else {
                                            return 203.0/3118.5;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                return 304.0/3256.6;
                            } else {
                                if ( rdb0_last_touched_diff <= 1375.5f ) {
                                    return 398.0/3142.9;
                                } else {
                                    return 476.0/3015.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.352799057961f ) {
                            if ( rdb0_last_touched_diff <= 657.0f ) {
                                return 239.0/2271.9;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.210482954979f ) {
                                        return 441.0/1677.0;
                                    } else {
                                        return 326.0/1717.6;
                                    }
                                } else {
                                    return 301.0/2546.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 465.0/1898.3;
                            } else {
                                return 239.0/1721.7;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 653.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0514253750443f ) {
                                return 208.0/3220.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.173859238625f ) {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            return 81.0/2129.8;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.405809223652f ) {
                                                return 59.0/2361.2;
                                            } else {
                                                return 33.0/2202.9;
                                            }
                                        }
                                    } else {
                                        return 38.0/2568.3;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.560991764069f ) {
                                            return 138.0/2718.6;
                                        } else {
                                            return 155.0/1912.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.202717661858f ) {
                                            return 96.0/2594.7;
                                        } else {
                                            return 83.0/3502.2;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.163290858269f ) {
                                if ( cl->stats.used_for_uip_creation <= 197.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0716831088066f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3730.5f ) {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 701.5f ) {
                                                    return 70.0/3004.8;
                                                } else {
                                                    return 62.0/2125.7;
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0412459075451f ) {
                                                    if ( cl->stats.used_for_uip_creation <= 24.5f ) {
                                                        return 55.0/2040.4;
                                                    } else {
                                                        return 65.0/3985.4;
                                                    }
                                                } else {
                                                    if ( cl->stats.glue <= 6.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 479.0f ) {
                                                            return 25.0/2379.5;
                                                        } else {
                                                            return 12.0/2816.0;
                                                        }
                                                    } else {
                                                        return 29.0/2113.5;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 69.0/2087.1;
                                        }
                                    } else {
                                        return 91.0/2600.8;
                                    }
                                } else {
                                    return 86.0/1987.6;
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 26.5f ) {
                                        return 63.0/2834.3;
                                    } else {
                                        return 36.0/4034.2;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 29.5f ) {
                                        return 40.0/2156.2;
                                    } else {
                                        if ( cl->size() <= 9.5f ) {
                                            return 23.0/2131.8;
                                        } else {
                                            return 17.0/3378.4;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.0199652779847f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                        if ( cl->stats.dump_number <= 18.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0374253354967f ) {
                                                    return 192.0/2097.3;
                                                } else {
                                                    return 167.0/2556.1;
                                                }
                                            } else {
                                                return 163.0/3002.8;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0337526202202f ) {
                                                return 161.0/2048.6;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3356.5f ) {
                                                    return 68.0/1892.2;
                                                } else {
                                                    return 99.0/1926.7;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                            return 113.0/2470.9;
                                        } else {
                                            return 64.0/2369.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                        if ( rdb0_last_touched_diff <= 2167.5f ) {
                                            return 106.0/3855.5;
                                        } else {
                                            return 91.0/2097.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                            if ( rdb0_last_touched_diff <= 1767.0f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.269364953041f ) {
                                                    return 43.0/1945.0;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 32.5f ) {
                                                        return 33.0/2738.9;
                                                    } else {
                                                        return 13.0/2091.2;
                                                    }
                                                }
                                            } else {
                                                return 86.0/3583.5;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                                return 93.0/2280.0;
                                            } else {
                                                return 38.0/2574.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 7.5f ) {
                                    if ( cl->size() <= 5.5f ) {
                                        return 111.0/1922.7;
                                    } else {
                                        return 68.0/2322.6;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                        if ( rdb0_last_touched_diff <= 1591.5f ) {
                                            return 208.0/3181.5;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.48933583498f ) {
                                                return 167.0/1837.4;
                                            } else {
                                                return 213.0/1591.7;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0917311608791f ) {
                                            return 106.0/1981.6;
                                        } else {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                return 105.0/2793.7;
                                            } else {
                                                return 85.0/3522.5;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 8687.0f ) {
                                    return 94.0/2125.7;
                                } else {
                                    return 142.0/2042.5;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.113111764193f ) {
                                    return 189.0/1737.9;
                                } else {
                                    if ( cl->size() <= 19.5f ) {
                                        return 145.0/1815.1;
                                    } else {
                                        return 153.0/2340.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                    if ( cl->size() <= 8.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0228907838464f ) {
                                return 336.0/1396.8;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.153614461422f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.073392868042f ) {
                                        return 234.0/1628.3;
                                    } else {
                                        return 234.0/1650.6;
                                    }
                                } else {
                                    if ( cl->size() <= 5.5f ) {
                                        return 281.0/1673.0;
                                    } else {
                                        return 417.0/1908.5;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 7835.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.084890268743f ) {
                                    return 255.0/2560.2;
                                } else {
                                    return 153.0/2403.9;
                                }
                            } else {
                                return 276.0/2365.3;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0795167535543f ) {
                            if ( cl->size() <= 12.5f ) {
                                return 526.0/2503.3;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                        return 300.0/1465.9;
                                    } else {
                                        return 342.0/1545.0;
                                    }
                                } else {
                                    return 188.0/1857.7;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.341791570187f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                        return 317.0/1459.8;
                                    } else {
                                        return 807.0/2308.4;
                                    }
                                } else {
                                    return 865.0/2367.3;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.506358385086f ) {
                                    return 379.0/2040.4;
                                } else {
                                    return 356.0/2351.1;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.737646818161f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        return 280.0/2562.2;
                                    } else {
                                        return 239.0/1831.3;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.157627880573f ) {
                                        return 266.0/1573.5;
                                    } else {
                                        return 289.0/1591.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 10764.0f ) {
                                    return 239.0/2897.2;
                                } else {
                                    return 268.0/2064.8;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                return 376.0/1591.7;
                            } else {
                                return 314.0/1965.3;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.255492866039f ) {
                            if ( cl->stats.glue_rel_queue <= 0.180090099573f ) {
                                return 179.0/1953.1;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0983050987124f ) {
                                    if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.325150370598f ) {
                                                    return 156.0/2064.8;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.427734225988f ) {
                                                        return 99.0/1847.6;
                                                    } else {
                                                        return 94.0/1985.6;
                                                    }
                                                }
                                            } else {
                                                return 126.0/2744.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                                return 191.0/2188.6;
                                            } else {
                                                return 141.0/2156.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.20353269577f ) {
                                            return 104.0/2066.8;
                                        } else {
                                            return 77.0/2178.5;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 6152.0f ) {
                                        return 193.0/3368.2;
                                    } else {
                                        return 186.0/1784.6;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.125131160021f ) {
                                    return 219.0/1668.9;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        return 246.0/1689.2;
                                    } else {
                                        return 180.0/2499.3;
                                    }
                                }
                            } else {
                                return 99.0/1902.4;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 10287.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0445717796683f ) {
                                return 525.0/1297.4;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue <= 4.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                            return 367.0/1709.5;
                                        } else {
                                            return 400.0/1504.4;
                                        }
                                    } else {
                                        return 533.0/1675.0;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 3.5f ) {
                                        return 442.0/1622.2;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4023.0f ) {
                                            return 422.0/1285.2;
                                        } else {
                                            return 818.0/2062.8;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 13.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0501415207982f ) {
                                            return 473.0/1218.2;
                                        } else {
                                            return 693.0/2472.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4640.0f ) {
                                            return 519.0/1106.5;
                                        } else {
                                            return 491.0/1165.4;
                                        }
                                    }
                                } else {
                                    return 644.0/962.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.683482527733f ) {
                                    return 904.0/1851.6;
                                } else {
                                    if ( cl->stats.dump_number <= 1.5f ) {
                                        return 927.0/615.2;
                                    } else {
                                        return 616.0/1220.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 1.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.647365570068f ) {
                                return 678.0/2519.6;
                            } else {
                                return 464.0/1311.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 4440.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.413377642632f ) {
                                            return 272.0/1865.8;
                                        } else {
                                            return 328.0/1835.4;
                                        }
                                    } else {
                                        return 561.0/2574.4;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.402677714825f ) {
                                        return 437.0/2050.6;
                                    } else {
                                        return 562.0/1912.5;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 38.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3105.0f ) {
                                            return 189.0/1890.2;
                                        } else {
                                            return 249.0/1620.2;
                                        }
                                    } else {
                                        if ( cl->size() <= 7.5f ) {
                                            return 238.0/1705.4;
                                        } else {
                                            return 475.0/2860.7;
                                        }
                                    }
                                } else {
                                    return 189.0/1855.7;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 3.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 34.5f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 19047.0f ) {
                                    return 709.0/1932.8;
                                } else {
                                    return 557.0/1145.1;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 18239.5f ) {
                                    return 621.0/1346.1;
                                } else {
                                    return 769.0/1082.1;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 672.0/1289.2;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 736.0/956.3;
                                    } else {
                                        return 749.0/633.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 96.5f ) {
                                    return 933.0/775.6;
                                } else {
                                    return 823.0/460.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.700801134109f ) {
                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0357229933143f ) {
                                    return 552.0/1210.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0455710962415f ) {
                                        return 372.0/1522.7;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            if ( rdb0_last_touched_diff <= 34850.0f ) {
                                                if ( cl->stats.dump_number <= 43.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.341895461082f ) {
                                                        return 359.0/1528.8;
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0989313274622f ) {
                                                            return 432.0/1400.9;
                                                        } else {
                                                            return 557.0/2087.1;
                                                        }
                                                    }
                                                } else {
                                                    return 536.0/1577.5;
                                                }
                                            } else {
                                                return 477.0/1189.7;
                                            }
                                        } else {
                                            return 738.0/1867.9;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 27916.5f ) {
                                    if ( cl->stats.size_rel <= 0.176139786839f ) {
                                        return 397.0/1301.4;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 727.0/1924.7;
                                        } else {
                                            return 842.0/1851.6;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 34573.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                            return 736.0/1536.9;
                                        } else {
                                            return 801.0/1382.6;
                                        }
                                    } else {
                                        return 895.0/2020.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.697094082832f ) {
                                return 650.0/1031.4;
                            } else {
                                return 642.0/771.5;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 140.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.894280731678f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 6297.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.487085461617f ) {
                                    return 543.0/2087.1;
                                } else {
                                    return 744.0/2170.4;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.204170972109f ) {
                                    return 964.0/2091.2;
                                } else {
                                    return 899.0/1467.9;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                return 557.0/1262.8;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 993.0/1285.2;
                                } else {
                                    return 833.0/1139.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1065.0/779.6;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.267857134342f ) {
                                        return 766.0/578.6;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 84.5f ) {
                                            return 1497.0/584.7;
                                        } else {
                                            return 1678.0/355.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.799341082573f ) {
                                    return 654.0/1352.2;
                                } else {
                                    return 768.0/1088.2;
                                }
                            }
                        } else {
                            return 589.0/1449.6;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->size() <= 215.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.988910138607f ) {
                                    if ( cl->size() <= 41.5f ) {
                                        return 645.0/767.4;
                                    } else {
                                        return 675.0/771.5;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 30.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.18019092083f ) {
                                            return 927.0/456.8;
                                        } else {
                                            return 915.0/343.1;
                                        }
                                    } else {
                                        return 896.0/249.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 356.5f ) {
                                    if ( cl->stats.size_rel <= 0.857158541679f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.535222172737f ) {
                                            return 802.0/464.9;
                                        } else {
                                            return 875.0/349.2;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 220.5f ) {
                                            return 1154.0/404.0;
                                        } else {
                                            return 976.0/237.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 19.5f ) {
                                        return 1111.0/276.1;
                                    } else {
                                        return 1897.0/263.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.22730791569f ) {
                                return 926.0/196.9;
                            } else {
                                return 1044.0/121.8;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            return 1380.0/1307.5;
                        } else {
                            return 448.0/1206.0;
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.glue <= 8.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 42311.0f ) {
                                return 559.0/1380.6;
                            } else {
                                return 667.0/1486.2;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.461288511753f ) {
                                return 1037.0/1953.1;
                            } else {
                                return 875.0/1049.7;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.417391359806f ) {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0256389342248f ) {
                                    return 707.0/1019.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 63846.5f ) {
                                        return 489.0/1287.2;
                                    } else {
                                        return 621.0/986.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.322828769684f ) {
                                    return 895.0/759.3;
                                } else {
                                    return 781.0/931.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 51020.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.189077854156f ) {
                                        return 841.0/1733.9;
                                    } else {
                                        return 961.0/1482.1;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.580031573772f ) {
                                        return 959.0/1159.3;
                                    } else {
                                        return 695.0/678.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.133268475533f ) {
                                            return 602.0/950.2;
                                        } else {
                                            return 872.0/1141.0;
                                        }
                                    } else {
                                        return 1236.0/781.7;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                        return 992.0/929.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.291649073362f ) {
                                            return 782.0/497.4;
                                        } else {
                                            return 813.0/383.7;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.218557924032f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.260734081268f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.418235123158f ) {
                                        return 964.0/1378.6;
                                    } else {
                                        return 690.0/777.6;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 31.5f ) {
                                        return 707.0/753.2;
                                    } else {
                                        return 801.0/643.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 12.5f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 744.0/899.4;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                            return 730.0/832.4;
                                        } else {
                                            return 871.0/763.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.size_rel <= 0.179080814123f ) {
                                            return 806.0/952.2;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 818.0/603.0;
                                            } else {
                                                return 819.0/732.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.493057012558f ) {
                                            if ( cl->size() <= 8.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.175362944603f ) {
                                                    return 801.0/672.0;
                                                } else {
                                                    return 793.0/529.9;
                                                }
                                            } else {
                                                return 1304.0/838.5;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 195993.5f ) {
                                                return 890.0/570.5;
                                            } else {
                                                return 1470.0/570.5;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.0772165507078f ) {
                                return 1096.0/460.9;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.68220603466f ) {
                                    if ( cl->size() <= 11.5f ) {
                                        return 844.0/787.8;
                                    } else {
                                        return 841.0/426.4;
                                    }
                                } else {
                                    return 1049.0/527.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            if ( rdb0_last_touched_diff <= 213600.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.237187504768f ) {
                                        return 720.0/848.7;
                                    } else {
                                        return 601.0/919.7;
                                    }
                                } else {
                                    return 859.0/641.6;
                                }
                            } else {
                                return 849.0/438.5;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 24.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.309537857771f ) {
                                        return 909.0/708.6;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 95662.5f ) {
                                            return 1171.0/718.7;
                                        } else {
                                            return 948.0/391.8;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.436599105597f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 117496.5f ) {
                                            return 811.0/489.3;
                                        } else {
                                            return 899.0/278.1;
                                        }
                                    } else {
                                        return 1770.0/511.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.632480382919f ) {
                                    if ( rdb0_last_touched_diff <= 282026.5f ) {
                                        return 871.0/400.0;
                                    } else {
                                        return 948.0/310.6;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.412703961134f ) {
                                            return 892.0/278.1;
                                        } else {
                                            return 1055.0/270.0;
                                        }
                                    } else {
                                        return 1191.0/251.8;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 28.5f ) {
                    if ( cl->stats.size_rel <= 0.870539069176f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 100762.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.719877481461f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 72501.5f ) {
                                    if ( cl->stats.dump_number <= 8.5f ) {
                                        return 966.0/1023.3;
                                    } else {
                                        return 866.0/1313.6;
                                    }
                                } else {
                                    return 732.0/607.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    return 1129.0/1059.8;
                                } else {
                                    if ( cl->stats.size_rel <= 0.708277702332f ) {
                                        if ( cl->stats.glue_rel_long <= 0.913117527962f ) {
                                            return 871.0/629.4;
                                        } else {
                                            return 1041.0/633.4;
                                        }
                                    } else {
                                        return 847.0/416.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 172670.5f ) {
                                if ( cl->stats.size_rel <= 0.531580746174f ) {
                                    return 1466.0/929.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.203934818506f ) {
                                        return 929.0/333.0;
                                    } else {
                                        return 857.0/402.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.499905347824f ) {
                                    if ( rdb0_last_touched_diff <= 336923.0f ) {
                                        return 1015.0/454.8;
                                    } else {
                                        return 1046.0/280.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.938917696476f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                            return 1084.0/268.0;
                                        } else {
                                            return 1124.0/357.3;
                                        }
                                    } else {
                                        return 992.0/170.5;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.188879519701f ) {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                    return 882.0/511.6;
                                } else {
                                    return 1108.0/513.7;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 116486.5f ) {
                                    return 1249.0/536.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        return 1053.0/294.4;
                                    } else {
                                        return 1486.0/257.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 54.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.400610625744f ) {
                                    return 1035.0/326.9;
                                } else {
                                    return 1057.0/150.2;
                                }
                            } else {
                                return 1744.0/268.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.9077911973f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                            if ( rdb0_last_touched_diff <= 61338.5f ) {
                                return 1218.0/1070.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.757580935955f ) {
                                    return 902.0/564.4;
                                } else {
                                    return 958.0/404.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 138.5f ) {
                                if ( cl->stats.size_rel <= 0.776067316532f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 52587.5f ) {
                                        return 919.0/603.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.193766593933f ) {
                                            return 1000.0/215.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.643104195595f ) {
                                                return 1095.0/633.4;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.365143597126f ) {
                                                    return 949.0/215.2;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.781740069389f ) {
                                                        return 1175.0/450.7;
                                                    } else {
                                                        return 1244.0/300.5;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.699251592159f ) {
                                        return 1422.0/509.6;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 240456.0f ) {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                return 897.0/339.1;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 5.8249168396f ) {
                                                    return 902.0/253.8;
                                                } else {
                                                    return 1130.0/233.5;
                                                }
                                            }
                                        } else {
                                            return 1414.0/194.9;
                                        }
                                    }
                                }
                            } else {
                                return 1097.0/170.5;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 80929.5f ) {
                            if ( cl->stats.glue <= 12.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 1277.0/686.2;
                                } else {
                                    if ( cl->stats.size_rel <= 0.89103782177f ) {
                                        return 1015.0/369.5;
                                    } else {
                                        return 893.0/211.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.04295754433f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        return 1005.0/426.4;
                                    } else {
                                        return 1666.0/377.6;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 7.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 210.5f ) {
                                            if ( rdb0_last_touched_diff <= 54867.5f ) {
                                                return 1786.0/347.2;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.31249260902f ) {
                                                    return 1362.0/127.9;
                                                } else {
                                                    return 936.0/144.2;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 52836.5f ) {
                                                return 1531.0/138.1;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 1384.0/52.8;
                                                } else {
                                                    return 1100.0/87.3;
                                                }
                                            }
                                        }
                                    } else {
                                        return 857.0/521.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 249.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.687656641006f ) {
                                    if ( cl->stats.glue_rel_long <= 1.1498606205f ) {
                                        if ( rdb0_last_touched_diff <= 142427.0f ) {
                                            return 1629.0/503.5;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 12.308391571f ) {
                                                return 1590.0/318.8;
                                            } else {
                                                return 1084.0/168.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 130942.5f ) {
                                            return 1364.0/253.8;
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 15.2209682465f ) {
                                                return 1212.0/156.3;
                                            } else {
                                                return 1047.0/83.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.16550838947f ) {
                                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->size() <= 22.5f ) {
                                                    return 1402.0/140.1;
                                                } else {
                                                    return 1290.0/231.5;
                                                }
                                            } else {
                                                return 1299.0/337.0;
                                            }
                                        } else {
                                            return 1262.0/123.8;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.10657596588f ) {
                                            return 1030.0/205.1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 95.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 193184.0f ) {
                                                    return 1522.0/77.2;
                                                } else {
                                                    return 1263.0/97.5;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.25848531723f ) {
                                                    return 988.0/129.9;
                                                } else {
                                                    return 1652.0/134.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.13670432568f ) {
                                    if ( cl->stats.num_antecedents_rel <= 1.46049189568f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 123748.0f ) {
                                            return 1242.0/166.5;
                                        } else {
                                            return 1978.0/142.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 16.5f ) {
                                            return 1027.0/221.3;
                                        } else {
                                            return 1345.0/178.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 39.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 162671.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 24.5774650574f ) {
                                                if ( cl->size() <= 54.5f ) {
                                                    return 997.0/123.8;
                                                } else {
                                                    return 1114.0/93.4;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 102962.5f ) {
                                                    return 1002.0/79.2;
                                                } else {
                                                    return 1966.0/103.5;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 242.5f ) {
                                                return 1125.0/30.5;
                                            } else {
                                                if ( cl->size() <= 77.5f ) {
                                                    return 1147.0/109.6;
                                                } else {
                                                    return 1269.0/30.5;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1000.0/115.7;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.822587966919f ) {
                if ( cl->stats.size_rel <= 0.699112415314f ) {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( rdb0_last_touched_diff <= 3578.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 421.0/2028.3;
                            } else {
                                return 419.0/1400.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                return 791.0/2048.6;
                            } else {
                                return 576.0/1118.7;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                            if ( rdb0_last_touched_diff <= 3720.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.455299526453f ) {
                                    return 221.0/1664.8;
                                } else {
                                    return 213.0/1744.0;
                                }
                            } else {
                                return 391.0/1504.4;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 18.5f ) {
                                return 206.0/2653.6;
                            } else {
                                return 74.0/1930.8;
                            }
                        }
                    }
                } else {
                    return 692.0/2239.4;
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.257807374001f ) {
                    return 445.0/1794.8;
                } else {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        return 548.0/958.3;
                    } else {
                        return 482.0/1906.4;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_6(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( rdb0_last_touched_diff <= 10013.5f ) {
            if ( cl->stats.size_rel <= 0.522766113281f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                            if ( cl->size() <= 6.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 2747.5f ) {
                                        if ( rdb0_last_touched_diff <= 580.5f ) {
                                            return 101.0/2174.4;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.321633815765f ) {
                                                return 176.0/1752.1;
                                            } else {
                                                return 191.0/2974.4;
                                            }
                                        }
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                return 356.0/2503.3;
                                            } else {
                                                return 241.0/3437.3;
                                            }
                                        } else {
                                            return 370.0/2566.3;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                                if ( cl->size() <= 4.5f ) {
                                                    return 81.0/1943.0;
                                                } else {
                                                    return 116.0/2087.1;
                                                }
                                            } else {
                                                return 111.0/3794.6;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0378637090325f ) {
                                                return 220.0/3053.5;
                                            } else {
                                                return 105.0/2925.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.166031301022f ) {
                                            return 137.0/2881.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0241933353245f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 1956.5f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 21.5f ) {
                                                        return 62.0/1947.0;
                                                    } else {
                                                        return 78.0/3678.9;
                                                    }
                                                } else {
                                                    return 104.0/1995.8;
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 2172.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.459947526455f ) {
                                                        if ( rdb0_last_touched_diff <= 576.5f ) {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0476889424026f ) {
                                                                return 14.0/2103.4;
                                                            } else {
                                                                return 5.0/2050.6;
                                                            }
                                                        } else {
                                                            return 40.0/3057.6;
                                                        }
                                                    } else {
                                                        return 42.0/2361.2;
                                                    }
                                                } else {
                                                    return 84.0/2513.5;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0530117489398f ) {
                                        return 223.0/1681.1;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 332.0/2566.3;
                                        } else {
                                            return 92.0/2588.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                        return 86.0/1953.1;
                                    } else {
                                        return 42.0/2060.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 3130.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2990.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                return 187.0/1754.2;
                                            } else {
                                                return 96.0/2474.9;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 431.5f ) {
                                                return 187.0/3262.7;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.171867668629f ) {
                                                    return 190.0/1967.3;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                        if ( cl->size() <= 10.5f ) {
                                                            return 240.0/1884.1;
                                                        } else {
                                                            return 315.0/1630.3;
                                                        }
                                                    } else {
                                                        return 187.0/2204.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                                if ( cl->stats.glue <= 5.5f ) {
                                                    return 353.0/1941.0;
                                                } else {
                                                    return 423.0/1776.5;
                                                }
                                            } else {
                                                return 520.0/1796.8;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 11080.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0879105329514f ) {
                                                    return 202.0/1918.6;
                                                } else {
                                                    return 226.0/1650.6;
                                                }
                                            } else {
                                                return 300.0/1815.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 32.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                return 273.0/2730.7;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.262373387814f ) {
                                                    return 135.0/1967.3;
                                                } else {
                                                    return 95.0/1884.1;
                                                }
                                            }
                                        } else {
                                            return 176.0/1762.3;
                                        }
                                    } else {
                                        return 194.0/1774.5;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( rdb0_last_touched_diff <= 2865.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 2870.0f ) {
                                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.106523901224f ) {
                                                    return 136.0/2064.8;
                                                } else {
                                                    return 125.0/3944.8;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0611068382859f ) {
                                                    return 104.0/2864.7;
                                                } else {
                                                    if ( cl->stats.num_antecedents_rel <= 0.130113691092f ) {
                                                        return 65.0/2982.5;
                                                    } else {
                                                        if ( cl->stats.size_rel <= 0.308639645576f ) {
                                                            return 58.0/3871.8;
                                                        } else {
                                                            return 17.0/2491.2;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1031.0f ) {
                                                return 99.0/2188.6;
                                            } else {
                                                return 138.0/2087.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 8.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.100505925715f ) {
                                                return 159.0/1898.3;
                                            } else {
                                                return 185.0/3112.4;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 19.5f ) {
                                                if ( cl->stats.size_rel <= 0.31043857336f ) {
                                                    return 165.0/1861.8;
                                                } else {
                                                    return 254.0/2141.9;
                                                }
                                            } else {
                                                return 142.0/2334.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3513.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.164183676243f ) {
                                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                return 118.0/3291.1;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 912.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.326038479805f ) {
                                                        return 45.0/2590.6;
                                                    } else {
                                                        return 35.0/4007.8;
                                                    }
                                                } else {
                                                    return 92.0/3244.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.136050790548f ) {
                                                return 156.0/3311.4;
                                            } else {
                                                return 68.0/3431.2;
                                            }
                                        }
                                    } else {
                                        return 149.0/3191.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 36342.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 11747.0f ) {
                                    return 288.0/2087.1;
                                } else {
                                    if ( rdb0_last_touched_diff <= 3376.0f ) {
                                        return 442.0/2539.9;
                                    } else {
                                        return 494.0/1705.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                    return 797.0/2495.2;
                                } else {
                                    return 550.0/1122.7;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.550274610519f ) {
                                    if ( rdb0_last_touched_diff <= 3609.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                            return 192.0/2550.0;
                                        } else {
                                            return 350.0/3102.3;
                                        }
                                    } else {
                                        return 310.0/1606.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1365.5f ) {
                                        return 181.0/1727.8;
                                    } else {
                                        return 369.0/1969.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                    if ( rdb0_last_touched_diff <= 1221.0f ) {
                                        return 70.0/2133.8;
                                    } else {
                                        return 189.0/1786.7;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1575.5f ) {
                                        if ( cl->stats.dump_number <= 4.5f ) {
                                            return 61.0/2229.3;
                                        } else {
                                            return 75.0/3691.1;
                                        }
                                    } else {
                                        return 139.0/2018.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                return 465.0/1638.4;
                            } else {
                                return 260.0/2251.6;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.1380032897f ) {
                                return 238.0/3563.1;
                            } else {
                                return 197.0/1737.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            return 312.0/2635.3;
                        } else {
                            return 655.0/2493.2;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.121471971273f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0413223132491f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.019519418478f ) {
                                return 168.0/1800.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 5114.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        return 165.0/2253.6;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 18.5f ) {
                                            return 119.0/3181.5;
                                        } else {
                                            return 30.0/2229.3;
                                        }
                                    }
                                } else {
                                    return 267.0/1713.6;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 1717.0f ) {
                                return 167.0/1993.7;
                            } else {
                                return 439.0/2046.5;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.669180512428f ) {
                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                return 447.0/2385.6;
                            } else {
                                return 114.0/2213.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.200708329678f ) {
                                return 471.0/2627.2;
                            } else {
                                return 305.0/2192.7;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 7775.0f ) {
                        if ( rdb0_last_touched_diff <= 4060.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 391.0/2440.4;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.271456032991f ) {
                                    return 131.0/1845.5;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            return 127.0/2799.8;
                                        } else {
                                            return 251.0/2976.4;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 47.5f ) {
                                            return 28.0/2182.6;
                                        } else {
                                            return 68.0/2440.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 136.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.04500007629f ) {
                                    return 365.0/2578.5;
                                } else {
                                    return 279.0/1571.4;
                                }
                            } else {
                                return 366.0/1658.7;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.978300213814f ) {
                            if ( cl->stats.glue_rel_queue <= 0.926283419132f ) {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( rdb0_last_touched_diff <= 2998.0f ) {
                                        return 327.0/1790.7;
                                    } else {
                                        return 722.0/1995.8;
                                    }
                                } else {
                                    return 311.0/3122.6;
                                }
                            } else {
                                return 544.0/1835.4;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 41069.0f ) {
                                return 663.0/2809.9;
                            } else {
                                return 467.0/1139.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_last_touched_diff <= 21765.5f ) {
                if ( cl->stats.antec_num_total_lits_rel <= 0.38922315836f ) {
                    if ( cl->stats.size_rel <= 0.483476519585f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.317559480667f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 9098.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            return 457.0/1790.7;
                                        } else {
                                            return 363.0/2072.9;
                                        }
                                    } else {
                                        return 475.0/1480.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                        return 441.0/2925.6;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 4643.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.457927107811f ) {
                                                return 294.0/1882.1;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 11875.0f ) {
                                                    return 332.0/1474.0;
                                                } else {
                                                    return 294.0/1595.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                if ( rdb0_last_touched_diff <= 20542.0f ) {
                                                    return 367.0/1303.4;
                                                } else {
                                                    return 641.0/2625.2;
                                                }
                                            } else {
                                                return 373.0/1827.3;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0350211784244f ) {
                                    return 527.0/1348.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 12402.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0899933278561f ) {
                                                return 516.0/2298.3;
                                            } else {
                                                return 319.0/1900.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                                return 347.0/1467.9;
                                            } else {
                                                return 382.0/1392.8;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 17.5f ) {
                                            return 471.0/1437.4;
                                        } else {
                                            return 480.0/1167.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 6300.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        return 705.0/1748.1;
                                    } else {
                                        return 486.0/2941.9;
                                    }
                                } else {
                                    return 736.0/1977.5;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 761.0/1086.2;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 15670.5f ) {
                                            return 782.0/1849.6;
                                        } else {
                                            return 756.0/1277.0;
                                        }
                                    } else {
                                        return 628.0/1768.4;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.836456298828f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.192609131336f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.102951467037f ) {
                                        return 558.0/940.0;
                                    } else {
                                        return 786.0/1906.4;
                                    }
                                } else {
                                    return 675.0/2755.1;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 11196.0f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        return 424.0/1388.7;
                                    } else {
                                        return 938.0/2062.8;
                                    }
                                } else {
                                    return 746.0/1340.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 12575.0f ) {
                                if ( cl->size() <= 17.5f ) {
                                    return 496.0/1124.8;
                                } else {
                                    return 1089.0/1693.3;
                                }
                            } else {
                                return 677.0/875.1;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 212.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.glue <= 8.5f ) {
                                    return 645.0/1070.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.820385396481f ) {
                                        return 1205.0/720.8;
                                    } else {
                                        return 997.0/341.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.641551375389f ) {
                                    return 581.0/1474.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 511.0/1039.5;
                                    } else {
                                        return 584.0/889.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.888264715672f ) {
                                return 828.0/1258.8;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1229.0/601.0;
                                } else {
                                    return 1476.0/414.2;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.812363266945f ) {
                            if ( cl->stats.size_rel <= 0.643300890923f ) {
                                return 308.0/1618.1;
                            } else {
                                return 518.0/1360.3;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                return 833.0/1199.9;
                            } else {
                                return 465.0/1297.4;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.924927711487f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.271964550018f ) {
                        if ( cl->stats.size_rel <= 0.412638604641f ) {
                            if ( cl->stats.size_rel <= 0.0629739537835f ) {
                                return 523.0/1325.8;
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 45016.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.310938417912f ) {
                                            return 529.0/1124.8;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.214672416449f ) {
                                                return 508.0/1206.0;
                                            } else {
                                                return 486.0/1666.9;
                                            }
                                        }
                                    } else {
                                        return 1080.0/1644.5;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 51636.5f ) {
                                        return 820.0/1423.2;
                                    } else {
                                        return 1031.0/1189.7;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 69861.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.207487076521f ) {
                                    return 893.0/1421.2;
                                } else {
                                    return 1015.0/1273.0;
                                }
                            } else {
                                return 851.0/706.5;
                            }
                        }
                    } else {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 30854.5f ) {
                                return 509.0/1236.4;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.176874175668f ) {
                                    return 590.0/1007.0;
                                } else {
                                    return 903.0/1212.1;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 52287.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 94.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 718.0/1179.6;
                                    } else {
                                        return 675.0/824.3;
                                    }
                                } else {
                                    return 1105.0/1076.1;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 1188.0/1110.6;
                                } else {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        return 1498.0/972.5;
                                    } else {
                                        return 898.0/345.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.25944596529f ) {
                        if ( rdb0_last_touched_diff <= 52314.5f ) {
                            return 611.0/816.2;
                        } else {
                            return 887.0/682.2;
                        }
                    } else {
                        if ( cl->size() <= 15.5f ) {
                            return 984.0/625.3;
                        } else {
                            if ( cl->stats.dump_number <= 12.5f ) {
                                if ( cl->stats.glue <= 23.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 1726.0/562.4;
                                    } else {
                                        return 1457.0/333.0;
                                    }
                                } else {
                                    return 1820.0/146.2;
                                }
                            } else {
                                return 839.0/568.5;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.antecedents_glue_long_reds_var <= 0.259319901466f ) {
            if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                if ( cl->stats.size_rel <= 0.160140037537f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 46377.5f ) {
                        if ( rdb0_last_touched_diff <= 18411.5f ) {
                            return 399.0/2428.2;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.025635432452f ) {
                                return 608.0/1346.1;
                            } else {
                                return 485.0/1628.3;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 9.5f ) {
                            return 773.0/962.4;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0291662793607f ) {
                                if ( rdb0_last_touched_diff <= 159347.0f ) {
                                    return 729.0/913.6;
                                } else {
                                    return 1087.0/769.5;
                                }
                            } else {
                                return 884.0/1027.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 11.5f ) {
                        if ( rdb0_last_touched_diff <= 35079.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.551886558533f ) {
                                return 603.0/2217.1;
                            } else {
                                return 529.0/1313.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 48039.0f ) {
                                return 715.0/1268.9;
                            } else {
                                return 990.0/1116.7;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.408089578152f ) {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                return 704.0/1417.1;
                            } else {
                                return 1209.0/915.7;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 915.0/1388.7;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    return 930.0/793.8;
                                } else {
                                    if ( rdb0_last_touched_diff <= 211402.5f ) {
                                        return 977.0/830.4;
                                    } else {
                                        return 967.0/489.3;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.616059303284f ) {
                    if ( cl->stats.dump_number <= 17.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 30345.0f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 302.0/1622.2;
                                } else {
                                    if ( rdb0_last_touched_diff <= 16166.0f ) {
                                        return 435.0/1616.1;
                                    } else {
                                        if ( cl->stats.dump_number <= 3.5f ) {
                                            return 764.0/1206.0;
                                        } else {
                                            return 767.0/2164.3;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0762815773487f ) {
                                    return 760.0/605.0;
                                } else {
                                    return 1177.0/1333.9;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 103817.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0772939026356f ) {
                                    return 1052.0/966.4;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.41135340929f ) {
                                        return 546.0/1445.6;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 23284.5f ) {
                                            return 562.0/1137.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.213739708066f ) {
                                                return 875.0/1025.3;
                                            } else {
                                                return 890.0/802.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 914.0/550.2;
                                } else {
                                    return 912.0/406.1;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 150790.0f ) {
                            if ( cl->size() <= 9.5f ) {
                                return 1065.0/1693.3;
                            } else {
                                if ( rdb0_last_touched_diff <= 65458.0f ) {
                                    return 598.0/1053.7;
                                } else {
                                    return 1148.0/897.4;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.284739524126f ) {
                                if ( cl->stats.size_rel <= 0.128535985947f ) {
                                    return 885.0/621.3;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.113171726465f ) {
                                        return 836.0/365.5;
                                    } else {
                                        return 793.0/424.3;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    return 1046.0/475.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 261645.5f ) {
                                        return 1272.0/501.5;
                                    } else {
                                        return 1623.0/316.7;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 46643.0f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 10428.5f ) {
                            return 800.0/1811.0;
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.196489751339f ) {
                                return 1031.0/732.9;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.881131052971f ) {
                                    return 835.0/1197.9;
                                } else {
                                    return 912.0/804.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 123621.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.01513671875f ) {
                                if ( cl->stats.glue_rel_long <= 0.941674590111f ) {
                                    return 1336.0/1039.5;
                                } else {
                                    return 1228.0/598.9;
                                }
                            } else {
                                return 973.0/363.4;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.801400244236f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 270960.0f ) {
                                    return 1438.0/544.1;
                                } else {
                                    return 919.0/221.3;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 248562.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.137627989054f ) {
                                        return 936.0/290.3;
                                    } else {
                                        return 1546.0/330.9;
                                    }
                                } else {
                                    if ( cl->size() <= 27.5f ) {
                                        return 1002.0/113.7;
                                    } else {
                                        return 1209.0/205.1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( rdb0_last_touched_diff <= 55396.5f ) {
                    if ( cl->stats.size_rel <= 0.714789927006f ) {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                    return 825.0/2020.1;
                                } else {
                                    return 876.0/1364.4;
                                }
                            } else {
                                return 1144.0/1360.3;
                            }
                        } else {
                            return 1315.0/1356.2;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 24482.5f ) {
                            return 674.0/696.4;
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.785909354687f ) {
                                return 777.0/564.4;
                            } else {
                                return 837.0/420.3;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.70743060112f ) {
                            if ( cl->stats.dump_number <= 24.5f ) {
                                return 1270.0/1459.8;
                            } else {
                                return 1140.0/866.9;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.564911365509f ) {
                                return 952.0/686.2;
                            } else {
                                return 962.0/373.6;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 42.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 96496.0f ) {
                                    return 808.0/627.4;
                                } else {
                                    return 924.0/363.4;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                    return 1271.0/525.8;
                                } else {
                                    return 992.0/292.4;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.789786100388f ) {
                                if ( cl->stats.glue_rel_long <= 0.658561229706f ) {
                                    if ( cl->stats.size_rel <= 0.439759671688f ) {
                                        return 923.0/326.9;
                                    } else {
                                        return 1423.0/745.1;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 1335.0/491.3;
                                    } else {
                                        return 1777.0/448.7;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 133852.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 109.5f ) {
                                        return 1048.0/357.3;
                                    } else {
                                        return 906.0/379.7;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.941918075085f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            return 1226.0/270.0;
                                        } else {
                                            return 1128.0/127.9;
                                        }
                                    } else {
                                        return 961.0/231.5;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.708259880543f ) {
                    if ( rdb0_last_touched_diff <= 32548.0f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.412327587605f ) {
                                    return 1134.0/442.6;
                                } else {
                                    return 992.0/213.2;
                                }
                            } else {
                                return 1094.0/1126.8;
                            }
                        } else {
                            return 637.0/1072.0;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 27.373878479f ) {
                            if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                if ( cl->stats.size_rel <= 0.558540344238f ) {
                                    return 863.0/566.4;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.909909188747f ) {
                                        return 1284.0/507.6;
                                    } else {
                                        return 1391.0/373.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.773295521736f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 1087.0/576.6;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.60422420502f ) {
                                            return 973.0/389.8;
                                        } else {
                                            return 1490.0/446.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 145889.5f ) {
                                        if ( cl->stats.glue <= 11.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.470625042915f ) {
                                                return 1121.0/306.6;
                                            } else {
                                                return 915.0/349.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                                return 1172.0/363.4;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 95.5f ) {
                                                    if ( cl->stats.size_rel <= 0.98798418045f ) {
                                                        return 1614.0/442.6;
                                                    } else {
                                                        return 1138.0/186.8;
                                                    }
                                                } else {
                                                    return 1125.0/152.3;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.18334749341f ) {
                                            return 1018.0/81.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.908024430275f ) {
                                                return 1283.0/306.6;
                                            } else {
                                                if ( cl->stats.size_rel <= 1.21789085865f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 1159.0/89.3;
                                                    } else {
                                                        return 1228.0/176.6;
                                                    }
                                                } else {
                                                    return 1036.0/154.3;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 50607.5f ) {
                                return 1662.0/373.6;
                            } else {
                                if ( cl->stats.dump_number <= 25.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 120.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.22279286385f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 111140.0f ) {
                                                return 967.0/201.0;
                                            } else {
                                                return 1172.0/158.4;
                                            }
                                        } else {
                                            return 979.0/83.2;
                                        }
                                    } else {
                                        return 1666.0/127.9;
                                    }
                                } else {
                                    return 1363.0/229.4;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 13.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 1535.0/682.2;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.03740000725f ) {
                                    return 1474.0/316.7;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 1.45435810089f ) {
                                        return 1187.0/462.9;
                                    } else {
                                        return 1667.0/440.6;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 141499.0f ) {
                                    return 1360.0/207.1;
                                } else {
                                    return 1776.0/408.1;
                                }
                            } else {
                                return 1858.0/146.2;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( rdb0_last_touched_diff <= 18640.5f ) {
                                return 1119.0/359.4;
                            } else {
                                if ( cl->stats.glue <= 20.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 27.2003097534f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 826.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.938520312309f ) {
                                                return 934.0/231.5;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.869565010071f ) {
                                                    return 1770.0/294.4;
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 83747.5f ) {
                                                            return 1265.0/168.5;
                                                        } else {
                                                            return 1106.0/54.8;
                                                        }
                                                    } else {
                                                        return 1551.0/221.3;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 973.0/105.6;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 114776.0f ) {
                                            return 1153.0/278.1;
                                        } else {
                                            return 1081.0/178.7;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 36226.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 2.04950666428f ) {
                                                return 1420.0/274.1;
                                            } else {
                                                return 1007.0/121.8;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 18.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 55214.0f ) {
                                                    return 1167.0/97.5;
                                                } else {
                                                    if ( cl->size() <= 67.5f ) {
                                                        return 1354.0/71.1;
                                                    } else {
                                                        return 1761.0/40.6;
                                                    }
                                                }
                                            } else {
                                                return 1835.0/176.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 2.08020114899f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.17626094818f ) {
                                                return 936.0/205.1;
                                            } else {
                                                return 1193.0/134.0;
                                            }
                                        } else {
                                            return 1238.0/136.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.940407752991f ) {
                                return 1175.0/176.6;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.15784955025f ) {
                                    return 1421.0/121.8;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 1.42358076572f ) {
                                        return 1883.0/91.4;
                                    } else {
                                        return 1192.0/10.2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_7(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_last_touched_diff <= 23876.5f ) {
        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->size() <= 6.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 33757.0f ) {
                            if ( rdb0_last_touched_diff <= 7852.5f ) {
                                if ( rdb0_last_touched_diff <= 1997.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0561724863946f ) {
                                        if ( cl->stats.size_rel <= 0.0813074856997f ) {
                                            return 91.0/2024.2;
                                        } else {
                                            return 268.0/2040.4;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 831.5f ) {
                                            if ( rdb0_last_touched_diff <= 257.5f ) {
                                                return 71.0/2022.2;
                                            } else {
                                                return 127.0/2572.4;
                                            }
                                        } else {
                                            return 277.0/3620.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                            return 379.0/2192.7;
                                        } else {
                                            return 293.0/2089.2;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 10789.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                return 173.0/1794.8;
                                            } else {
                                                return 135.0/2066.8;
                                            }
                                        } else {
                                            return 311.0/2649.5;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0262290071696f ) {
                                    return 485.0/2022.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        return 381.0/2251.6;
                                    } else {
                                        return 444.0/2263.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 68466.0f ) {
                                return 413.0/2426.2;
                            } else {
                                return 320.0/1583.6;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.size_rel <= 0.301858514547f ) {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    return 884.0/2347.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        return 541.0/2584.6;
                                    } else {
                                        return 449.0/1400.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.140719622374f ) {
                                    return 482.0/2572.4;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.651683568954f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.22979195416f ) {
                                            return 336.0/1606.0;
                                        } else {
                                            return 564.0/2095.3;
                                        }
                                    } else {
                                        return 577.0/1876.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 33351.0f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.360787540674f ) {
                                        return 197.0/1827.3;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0959198921919f ) {
                                            return 229.0/1926.7;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                return 273.0/2468.8;
                                            } else {
                                                return 147.0/3140.8;
                                            }
                                        }
                                    }
                                } else {
                                    return 176.0/2578.5;
                                }
                            } else {
                                return 346.0/2682.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.46119555831f ) {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.166419744492f ) {
                                if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                    return 958.0/2190.7;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 17174.5f ) {
                                        return 258.0/1573.5;
                                    } else {
                                        return 438.0/1246.6;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.785304605961f ) {
                                        return 571.0/1244.6;
                                    } else {
                                        return 648.0/755.3;
                                    }
                                } else {
                                    if ( cl->size() <= 20.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                            return 829.0/1865.8;
                                        } else {
                                            return 736.0/2381.5;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 10001.5f ) {
                                            return 580.0/1262.8;
                                        } else {
                                            return 571.0/1480.1;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 37669.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0486241690814f ) {
                                        return 286.0/1691.2;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.615048468113f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                    return 257.0/2198.8;
                                                } else {
                                                    return 417.0/2744.9;
                                                }
                                            } else {
                                                if ( cl->size() <= 18.5f ) {
                                                    return 389.0/2271.9;
                                                } else {
                                                    return 447.0/2866.8;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 12.5f ) {
                                                return 208.0/3309.4;
                                            } else {
                                                return 101.0/2158.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0567486844957f ) {
                                        return 276.0/1650.6;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                            return 442.0/1508.5;
                                        } else {
                                            return 219.0/1737.9;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 2382.5f ) {
                                    return 190.0/2647.5;
                                } else {
                                    return 280.0/1792.7;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( rdb0_last_touched_diff <= 9799.5f ) {
                                return 355.0/1459.8;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.00547432899f ) {
                                    return 674.0/917.7;
                                } else {
                                    return 1065.0/361.4;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 28744.5f ) {
                                if ( rdb0_last_touched_diff <= 9444.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                        return 403.0/1772.4;
                                    } else {
                                        return 301.0/2692.2;
                                    }
                                } else {
                                    return 694.0/1409.0;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 4.0173459053f ) {
                                    return 429.0/1256.7;
                                } else {
                                    return 382.0/1333.9;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals <= 50.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.32259529829f ) {
                            if ( cl->stats.dump_number <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.124131947756f ) {
                                    return 895.0/1697.3;
                                } else {
                                    return 801.0/751.2;
                                }
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        if ( cl->stats.size_rel <= 0.233999013901f ) {
                                            if ( cl->stats.dump_number <= 14.5f ) {
                                                return 617.0/2141.9;
                                            } else {
                                                return 551.0/1526.8;
                                            }
                                        } else {
                                            return 610.0/2521.6;
                                        }
                                    } else {
                                        return 496.0/1358.3;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0592520348728f ) {
                                        return 744.0/1108.5;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.102546803653f ) {
                                            if ( rdb0_last_touched_diff <= 20441.5f ) {
                                                return 766.0/2505.4;
                                            } else {
                                                return 523.0/1139.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.51586329937f ) {
                                                return 620.0/1534.9;
                                            } else {
                                                return 777.0/1307.5;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.866261363029f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 637.0/1027.3;
                                } else {
                                    return 481.0/1287.2;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.977280020714f ) {
                                    return 666.0/763.4;
                                } else {
                                    return 881.0/592.8;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 13079.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                return 377.0/1644.5;
                            } else {
                                return 195.0/1934.9;
                            }
                        } else {
                            return 414.0/1551.1;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.875136733055f ) {
                        if ( rdb0_last_touched_diff <= 11799.5f ) {
                            return 532.0/1268.9;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.79157781601f ) {
                                return 604.0/1027.3;
                            } else {
                                return 810.0/763.4;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->stats.size_rel <= 1.14142680168f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 7.72107028961f ) {
                                    return 849.0/499.4;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 27.7130241394f ) {
                                        return 1017.0/294.4;
                                    } else {
                                        return 1076.0/182.7;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.40997338295f ) {
                                    return 1116.0/107.6;
                                } else {
                                    return 1003.0/174.6;
                                }
                            }
                        } else {
                            return 822.0/1260.8;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( rdb0_last_touched_diff <= 7904.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 447.0/2791.6;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.0942943543196f ) {
                                    return 220.0/1601.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.599671840668f ) {
                                        return 218.0/3126.6;
                                    } else {
                                        return 216.0/1884.1;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.730815768242f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0851135998964f ) {
                                        return 214.0/2980.5;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            return 117.0/2418.1;
                                        } else {
                                            return 80.0/2507.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.023749999702f ) {
                                        if ( cl->size() <= 13.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.443793356419f ) {
                                                return 115.0/1934.9;
                                            } else {
                                                return 127.0/1973.4;
                                            }
                                        } else {
                                            return 159.0/2058.7;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.278944909573f ) {
                                            return 123.0/1821.2;
                                        } else {
                                            return 318.0/3341.8;
                                        }
                                    }
                                }
                            } else {
                                return 244.0/2223.2;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 1840.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0718818455935f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 117.0/1959.2;
                                    } else {
                                        return 104.0/2791.6;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0847856327891f ) {
                                            if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                return 50.0/2087.1;
                                            } else {
                                                return 93.0/2334.8;
                                            }
                                        } else {
                                            return 199.0/3699.2;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.342459499836f ) {
                                            return 62.0/1983.6;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                                return 54.0/3423.1;
                                            } else {
                                                return 45.0/2016.1;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    if ( cl->stats.size_rel <= 0.417105436325f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                            return 71.0/2089.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.370458334684f ) {
                                                return 43.0/2044.5;
                                            } else {
                                                return 59.0/2184.6;
                                            }
                                        }
                                    } else {
                                        return 95.0/2115.6;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0652998238802f ) {
                                        if ( cl->stats.size_rel <= 0.0326898023486f ) {
                                            return 25.0/2131.8;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 97.5f ) {
                                                if ( rdb0_last_touched_diff <= 567.5f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 31.5f ) {
                                                        return 66.0/3242.4;
                                                    } else {
                                                        return 34.0/2872.9;
                                                    }
                                                } else {
                                                    return 120.0/3810.8;
                                                }
                                            } else {
                                                return 106.0/2054.6;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.328551113605f ) {
                                            if ( cl->stats.glue_rel_long <= 0.217975020409f ) {
                                                if ( cl->stats.size_rel <= 0.111506909132f ) {
                                                    return 46.0/2223.2;
                                                } else {
                                                    return 61.0/1949.1;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                    return 35.0/3488.0;
                                                } else {
                                                    return 64.0/2172.4;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.398830115795f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.107231356204f ) {
                                                    return 9.0/2247.5;
                                                } else {
                                                    return 23.0/2225.2;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                                                    if ( cl->stats.size_rel <= 0.184178337455f ) {
                                                        return 43.0/2083.1;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.127692401409f ) {
                                                                return 11.0/2582.5;
                                                            } else {
                                                                return 20.0/2099.3;
                                                            }
                                                        } else {
                                                            return 42.0/2824.1;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 34.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.263228952885f ) {
                                                            return 64.0/2036.4;
                                                        } else {
                                                            return 51.0/2726.7;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.777786552906f ) {
                                                            return 30.0/3090.1;
                                                        } else {
                                                            return 34.0/2188.6;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 3755.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0226748455316f ) {
                                        return 125.0/1977.5;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.526084542274f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0495610460639f ) {
                                                if ( rdb0_last_touched_diff <= 3404.5f ) {
                                                    return 70.0/2255.6;
                                                } else {
                                                    return 107.0/2113.5;
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                    return 40.0/2115.6;
                                                } else {
                                                    return 61.0/1930.8;
                                                }
                                            }
                                        } else {
                                            return 108.0/2148.0;
                                        }
                                    }
                                } else {
                                    return 177.0/2976.4;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.279340922832f ) {
                                    return 170.0/1943.0;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 14.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                                return 124.0/1888.2;
                                            } else {
                                                return 70.0/1955.2;
                                            }
                                        } else {
                                            if ( cl->size() <= 17.5f ) {
                                                return 203.0/2109.5;
                                            } else {
                                                return 215.0/3017.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.16517853737f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0553772300482f ) {
                                                return 101.0/2491.2;
                                            } else {
                                                return 72.0/2466.8;
                                            }
                                        } else {
                                            return 118.0/1914.6;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.477943003178f ) {
                        if ( rdb0_last_touched_diff <= 15144.0f ) {
                            if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                if ( cl->stats.dump_number <= 38.5f ) {
                                    if ( cl->size() <= 6.5f ) {
                                        return 294.0/2572.4;
                                    } else {
                                        return 285.0/1922.7;
                                    }
                                } else {
                                    return 155.0/1839.4;
                                }
                            } else {
                                if ( cl->size() <= 7.5f ) {
                                    return 320.0/2911.4;
                                } else {
                                    if ( cl->size() <= 14.5f ) {
                                        return 503.0/2960.2;
                                    } else {
                                        return 238.0/1666.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                return 324.0/1740.0;
                            } else {
                                return 353.0/1441.5;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.59029841423f ) {
                                return 492.0/2056.7;
                            } else {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 378.0/1356.2;
                                } else {
                                    return 669.0/1861.8;
                                }
                            }
                        } else {
                            return 247.0/1774.5;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 10763.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.257999837399f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 4398.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.417985320091f ) {
                                if ( rdb0_last_touched_diff <= 4908.0f ) {
                                    return 83.0/1926.7;
                                } else {
                                    return 219.0/2939.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0866978019476f ) {
                                    return 226.0/1959.2;
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 156.0/1841.5;
                                    } else {
                                        return 136.0/2008.0;
                                    }
                                }
                            }
                        } else {
                            return 234.0/1772.4;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 70.5f ) {
                            return 205.0/1693.3;
                        } else {
                            return 342.0/1790.7;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.843582391739f ) {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 4799.0f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    return 258.0/1930.8;
                                } else {
                                    return 336.0/1786.7;
                                }
                            } else {
                                return 603.0/2574.4;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 582.0/1482.1;
                            } else {
                                return 454.0/2028.3;
                            }
                        }
                    } else {
                        return 1092.0/1983.6;
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 11.5f ) {
            if ( rdb0_last_touched_diff <= 68632.5f ) {
                if ( cl->stats.glue_rel_long <= 0.757545351982f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.50269305706f ) {
                            if ( cl->stats.glue_rel_queue <= 0.262467622757f ) {
                                return 489.0/1031.4;
                            } else {
                                if ( cl->stats.size_rel <= 0.243495702744f ) {
                                    return 598.0/1683.1;
                                } else {
                                    return 528.0/1248.6;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                return 437.0/1299.4;
                            } else {
                                return 456.0/1626.3;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 44158.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( rdb0_last_touched_diff <= 31030.5f ) {
                                        return 544.0/1431.4;
                                    } else {
                                        return 737.0/1331.9;
                                    }
                                } else {
                                    return 594.0/927.8;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 32898.5f ) {
                                    if ( cl->stats.size_rel <= 0.157213136554f ) {
                                        return 461.0/1486.2;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.269035100937f ) {
                                            return 509.0/1173.5;
                                        } else {
                                            return 632.0/1804.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.163294434547f ) {
                                        return 496.0/1386.7;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.145438194275f ) {
                                            return 500.0/1157.3;
                                        } else {
                                            return 836.0/1664.8;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 55134.0f ) {
                                    if ( cl->size() <= 6.5f ) {
                                        return 915.0/1934.9;
                                    } else {
                                        return 604.0/1003.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0434292852879f ) {
                                        return 631.0/860.8;
                                    } else {
                                        return 850.0/1392.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    return 749.0/832.4;
                                } else {
                                    return 809.0/1007.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.9255399704f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 32614.5f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                return 559.0/1311.6;
                            } else {
                                return 792.0/1287.2;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                return 598.0/962.4;
                            } else {
                                return 703.0/722.8;
                            }
                        }
                    } else {
                        return 978.0/659.8;
                    }
                }
            } else {
                if ( cl->stats.glue <= 6.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.206781610847f ) {
                            return 824.0/718.7;
                        } else {
                            if ( cl->size() <= 4.5f ) {
                                return 1011.0/1490.2;
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 768.0/1045.6;
                                } else {
                                    return 1011.0/871.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 163357.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.187105923891f ) {
                                    return 808.0/1303.4;
                                } else {
                                    return 838.0/996.9;
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.166712984443f ) {
                                    return 1091.0/1364.4;
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        return 785.0/848.7;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 113567.5f ) {
                                            return 1240.0/1169.4;
                                        } else {
                                            return 783.0/576.6;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 269346.0f ) {
                                    return 971.0/862.9;
                                } else {
                                    return 784.0/534.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 61.5f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        return 978.0/645.6;
                                    } else {
                                        return 1192.0/412.1;
                                    }
                                } else {
                                    return 772.0/515.7;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 30.5f ) {
                        if ( rdb0_last_touched_diff <= 179972.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.220457702875f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 97508.0f ) {
                                    return 847.0/1005.0;
                                } else {
                                    return 887.0/621.3;
                                }
                            } else {
                                return 1174.0/911.6;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.159438312054f ) {
                                return 841.0/452.8;
                            } else {
                                return 1353.0/424.3;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.768295764923f ) {
                            return 786.0/493.4;
                        } else {
                            return 1789.0/529.9;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.893357098103f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 63650.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.717531740665f ) {
                        if ( cl->size() <= 99.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.01513671875f ) {
                                if ( cl->stats.dump_number <= 6.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 28367.5f ) {
                                        return 595.0/1112.6;
                                    } else {
                                        return 762.0/812.1;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.393249243498f ) {
                                        return 554.0/1238.5;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.567303538322f ) {
                                            return 726.0/1364.4;
                                        } else {
                                            return 640.0/1033.4;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 17.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 648.0/1204.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.825518608093f ) {
                                            return 791.0/810.1;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.23438128829f ) {
                                                return 648.0/789.8;
                                            } else {
                                                return 607.0/885.2;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.291690409184f ) {
                                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                            if ( cl->stats.size_rel <= 0.61967664957f ) {
                                                return 584.0/936.0;
                                            } else {
                                                return 603.0/856.8;
                                            }
                                        } else {
                                            return 1147.0/1011.1;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                            return 975.0/1011.1;
                                        } else {
                                            return 932.0/670.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            return 842.0/450.7;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 7.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                return 1040.0/1244.6;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    return 1161.0/885.2;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        if ( cl->size() <= 25.5f ) {
                                            return 1082.0/663.9;
                                        } else {
                                            return 1027.0/367.5;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.464207053185f ) {
                                            return 988.0/231.5;
                                        } else {
                                            return 982.0/324.8;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 32.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.104158267379f ) {
                                    return 600.0/966.4;
                                } else {
                                    return 678.0/887.2;
                                }
                            } else {
                                return 1023.0/1106.5;
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            return 1015.0/923.8;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 120298.0f ) {
                                if ( cl->size() <= 16.5f ) {
                                    return 1026.0/777.6;
                                } else {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        return 1371.0/562.4;
                                    } else {
                                        return 805.0/572.5;
                                    }
                                }
                            } else {
                                return 1117.0/339.1;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.875648736954f ) {
                            if ( cl->stats.glue_rel_long <= 0.453296452761f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 1169.0/609.1;
                                    } else {
                                        return 992.0/351.2;
                                    }
                                } else {
                                    return 1372.0/1041.5;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 180226.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 49.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 1278.0/822.3;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 138887.5f ) {
                                                return 996.0/511.6;
                                            } else {
                                                return 863.0/351.2;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.190702915192f ) {
                                            return 765.0/479.1;
                                        } else {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.71689748764f ) {
                                                    return 1114.0/412.1;
                                                } else {
                                                    return 1680.0/412.1;
                                                }
                                            } else {
                                                return 860.0/428.4;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 67.5f ) {
                                        if ( rdb0_last_touched_diff <= 249453.0f ) {
                                            if ( cl->stats.dump_number <= 22.5f ) {
                                                return 1112.0/241.6;
                                            } else {
                                                return 1063.0/422.3;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.736054420471f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.599472999573f ) {
                                                    return 940.0/225.4;
                                                } else {
                                                    return 1197.0/209.1;
                                                }
                                            } else {
                                                return 1610.0/442.6;
                                            }
                                        }
                                    } else {
                                        return 1382.0/556.3;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.14220751822f ) {
                                    return 995.0/402.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 156739.5f ) {
                                        if ( cl->stats.size_rel <= 1.12153875828f ) {
                                            return 918.0/339.1;
                                        } else {
                                            return 1111.0/310.6;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 64.5f ) {
                                            return 1387.0/322.8;
                                        } else {
                                            return 995.0/207.1;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.708940625191f ) {
                                    return 1399.0/339.1;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 221416.0f ) {
                                        return 1390.0/282.2;
                                    } else {
                                        return 1087.0/115.7;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.194295331836f ) {
                    if ( rdb0_last_touched_diff <= 84914.0f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                if ( cl->stats.size_rel <= 0.722348332405f ) {
                                    return 767.0/544.1;
                                } else {
                                    return 907.0/456.8;
                                }
                            } else {
                                return 896.0/1258.8;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.29206228256f ) {
                                if ( cl->stats.num_overlap_literals <= 22.5f ) {
                                    return 1318.0/895.4;
                                } else {
                                    return 936.0/294.4;
                                }
                            } else {
                                return 1045.0/276.1;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.01999998093f ) {
                            if ( rdb0_last_touched_diff <= 150694.5f ) {
                                return 1549.0/741.1;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.14733707905f ) {
                                    return 1737.0/471.0;
                                } else {
                                    return 1024.0/156.3;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                return 1269.0/292.4;
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.13425064087f ) {
                                    return 1047.0/182.7;
                                } else {
                                    return 1132.0/77.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 180.5f ) {
                        if ( rdb0_last_touched_diff <= 60654.5f ) {
                            if ( cl->stats.dump_number <= 5.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.973512887955f ) {
                                    return 1218.0/690.3;
                                } else {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        return 1411.0/637.5;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 35404.5f ) {
                                            return 1662.0/440.6;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.11093533039f ) {
                                                return 923.0/205.1;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 14.9897956848f ) {
                                                    return 934.0/209.1;
                                                } else {
                                                    return 1318.0/140.1;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 33892.0f ) {
                                    return 734.0/1031.4;
                                } else {
                                    return 732.0/598.9;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.65033364296f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 1777.0/548.2;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 132822.5f ) {
                                            if ( cl->stats.dump_number <= 9.5f ) {
                                                return 1434.0/261.9;
                                            } else {
                                                return 1177.0/377.6;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 197002.5f ) {
                                                    return 1000.0/103.5;
                                                } else {
                                                    return 1352.0/196.9;
                                                }
                                            } else {
                                                return 1632.0/343.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 9.55777740479f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 76272.0f ) {
                                            return 1061.0/351.2;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 178587.5f ) {
                                                return 1842.0/402.0;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    return 970.0/99.5;
                                                } else {
                                                    return 1050.0/205.1;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 78680.0f ) {
                                            return 989.0/176.6;
                                        } else {
                                            if ( cl->stats.glue <= 17.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.20287823677f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.070140481f ) {
                                                        return 997.0/160.4;
                                                    } else {
                                                        return 1030.0/201.0;
                                                    }
                                                } else {
                                                    return 1237.0/127.9;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 1064.0/60.9;
                                                } else {
                                                    return 1356.0/156.3;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.746641874313f ) {
                                    if ( cl->stats.num_overlap_literals <= 50.5f ) {
                                        return 1800.0/152.3;
                                    } else {
                                        return 1220.0/241.6;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.57570433617f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 10.4368247986f ) {
                                            return 1155.0/117.8;
                                        } else {
                                            return 1165.0/71.1;
                                        }
                                    } else {
                                        return 1228.0/40.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 12.5f ) {
                            if ( cl->stats.num_overlap_literals <= 275.5f ) {
                                return 999.0/233.5;
                            } else {
                                return 1436.0/422.3;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 1442.0/412.1;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.17001509666f ) {
                                        if ( cl->stats.dump_number <= 22.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.01250243187f ) {
                                                    return 1546.0/235.5;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 38505.5f ) {
                                                        return 961.0/136.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 500.0f ) {
                                                            return 1243.0/75.1;
                                                        } else {
                                                            return 1270.0/42.6;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 1376.0/266.0;
                                            }
                                        } else {
                                            return 1435.0/290.3;
                                        }
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 17.3971977234f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.47716736794f ) {
                                                if ( cl->stats.size_rel <= 1.0810302496f ) {
                                                    return 960.0/180.7;
                                                } else {
                                                    return 1094.0/87.3;
                                                }
                                            } else {
                                                return 1206.0/93.4;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 1.9757361412f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 152890.5f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.52689480782f ) {
                                                            return 1079.0/69.0;
                                                        } else {
                                                            return 1452.0/138.1;
                                                        }
                                                    } else {
                                                        return 1228.0/140.1;
                                                    }
                                                } else {
                                                    return 1390.0/77.2;
                                                }
                                            } else {
                                                return 1912.0/81.2;
                                            }
                                        }
                                    }
                                } else {
                                    return 1861.0/79.2;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_8(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
        if ( rdb0_last_touched_diff <= 10829.5f ) {
            if ( cl->stats.glue_rel_queue <= 0.654399871826f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.00228067417629f ) {
                            return 36.0/2144.0;
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 51285.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 16524.5f ) {
                                                if ( rdb0_last_touched_diff <= 5343.5f ) {
                                                    return 271.0/2008.0;
                                                } else {
                                                    return 274.0/1543.0;
                                                }
                                            } else {
                                                return 289.0/1555.2;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.123019084334f ) {
                                                    return 357.0/2921.6;
                                                } else {
                                                    return 225.0/2568.3;
                                                }
                                            } else {
                                                return 286.0/2310.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 11672.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                    return 176.0/2994.7;
                                                } else {
                                                    return 134.0/1825.2;
                                                }
                                            } else {
                                                return 290.0/3301.2;
                                            }
                                        } else {
                                            return 89.0/2081.0;
                                        }
                                    }
                                } else {
                                    return 436.0/2038.4;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0575772412121f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                            if ( cl->stats.size_rel <= 0.101551741362f ) {
                                                return 129.0/1851.6;
                                            } else {
                                                return 195.0/2097.3;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0249926485121f ) {
                                                return 125.0/1906.4;
                                            } else {
                                                return 134.0/2783.5;
                                            }
                                        }
                                    } else {
                                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                            if ( rdb0_last_touched_diff <= 3526.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.149091944098f ) {
                                                    return 102.0/2075.0;
                                                } else {
                                                    return 85.0/2093.2;
                                                }
                                            } else {
                                                return 273.0/2878.9;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                return 159.0/3674.8;
                                            } else {
                                                return 64.0/3496.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0938562303782f ) {
                                            return 111.0/1955.2;
                                        } else {
                                            return 95.0/2566.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0225725062191f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.174825236201f ) {
                                                return 94.0/1975.5;
                                            } else {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    return 99.0/3140.8;
                                                } else {
                                                    return 61.0/3019.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.126387000084f ) {
                                                if ( rdb0_last_touched_diff <= 1159.0f ) {
                                                    return 44.0/3995.6;
                                                } else {
                                                    return 80.0/2460.7;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 0.187066078186f ) {
                                                    return 35.0/2005.9;
                                                } else {
                                                    return 26.0/3857.5;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00847205705941f ) {
                            if ( cl->stats.glue_rel_queue <= 0.525063991547f ) {
                                if ( cl->stats.glue_rel_long <= 0.493633568287f ) {
                                    if ( rdb0_last_touched_diff <= 3364.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                return 400.0/2282.0;
                                            } else {
                                                if ( cl->stats.dump_number <= 27.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0783217698336f ) {
                                                        return 172.0/2040.4;
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.211574971676f ) {
                                                            return 92.0/1910.5;
                                                        } else {
                                                            return 76.0/1977.5;
                                                        }
                                                    }
                                                } else {
                                                    return 160.0/2072.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.265017569065f ) {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.289737462997f ) {
                                                        return 172.0/3618.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 652.5f ) {
                                                            return 46.0/2052.6;
                                                        } else {
                                                            return 102.0/2328.7;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->size() <= 15.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.0971288233995f ) {
                                                            return 80.0/2263.8;
                                                        } else {
                                                            return 60.0/2623.1;
                                                        }
                                                    } else {
                                                        return 49.0/2807.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                                    return 48.0/2379.5;
                                                } else {
                                                    return 30.0/2529.7;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                return 444.0/1817.1;
                                            } else {
                                                return 274.0/1778.5;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.058794118464f ) {
                                                return 358.0/3280.9;
                                            } else {
                                                return 241.0/3291.1;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 269.0/2856.6;
                                    } else {
                                        return 143.0/2070.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 20.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 436.0/2379.5;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1330.5f ) {
                                            return 102.0/3496.1;
                                        } else {
                                            return 169.0/2862.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 5631.0f ) {
                                        return 192.0/2623.1;
                                    } else {
                                        return 263.0/1660.8;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.538288831711f ) {
                                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                            return 233.0/2012.0;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.375526189804f ) {
                                                return 113.0/2442.4;
                                            } else {
                                                return 79.0/2868.8;
                                            }
                                        }
                                    } else {
                                        return 163.0/2075.0;
                                    }
                                } else {
                                    return 245.0/1807.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.size_rel <= 0.523533344269f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                            return 373.0/2625.2;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 14.5f ) {
                                                return 143.0/2097.3;
                                            } else {
                                                return 78.0/2020.1;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 65.5f ) {
                                            return 184.0/1691.2;
                                        } else {
                                            return 228.0/1914.6;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1906.0f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 3.29931974411f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 3884.5f ) {
                                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                        return 54.0/2245.5;
                                                    } else {
                                                        return 88.0/2462.7;
                                                    }
                                                } else {
                                                    return 164.0/2247.5;
                                                }
                                            } else {
                                                return 137.0/2245.5;
                                            }
                                        } else {
                                            return 238.0/2499.3;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.500730276108f ) {
                                                    return 412.0/1331.9;
                                                } else {
                                                    return 508.0/1447.6;
                                                }
                                            } else {
                                                return 467.0/2592.7;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 5482.5f ) {
                                                return 208.0/3128.7;
                                            } else {
                                                return 248.0/1809.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 13283.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 4899.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                return 494.0/2722.6;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0382583960891f ) {
                                    return 98.0/1900.3;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.10430110991f ) {
                                        return 181.0/1949.1;
                                    } else {
                                        return 194.0/2811.9;
                                    }
                                }
                            }
                        } else {
                            return 422.0/2649.5;
                        }
                    } else {
                        return 566.0/1703.4;
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 35006.5f ) {
                        if ( rdb0_last_touched_diff <= 9516.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.size_rel <= 0.29599660635f ) {
                                        return 417.0/1851.6;
                                    } else {
                                        return 366.0/2150.1;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 190.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            if ( rdb0_last_touched_diff <= 3512.5f ) {
                                                return 288.0/1640.5;
                                            } else {
                                                return 485.0/1845.5;
                                            }
                                        } else {
                                            return 679.0/2253.6;
                                        }
                                    } else {
                                        return 460.0/1126.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 49.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0660687834024f ) {
                                        return 215.0/1646.6;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.384549260139f ) {
                                            if ( rdb0_last_touched_diff <= 3628.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                    return 170.0/1807.0;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                                        return 123.0/1953.1;
                                                    } else {
                                                        return 104.0/2458.7;
                                                    }
                                                }
                                            } else {
                                                return 345.0/2669.8;
                                            }
                                        } else {
                                            return 204.0/1646.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                        return 370.0/2010.0;
                                    } else {
                                        return 192.0/1918.6;
                                    }
                                }
                            }
                        } else {
                            return 887.0/1561.3;
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                            if ( rdb0_last_touched_diff <= 3250.5f ) {
                                return 673.0/2162.3;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.856587171555f ) {
                                    return 556.0/1074.0;
                                } else {
                                    return 620.0/903.5;
                                }
                            }
                        } else {
                            return 408.0/3134.8;
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 3233.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0621646530926f ) {
                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                return 161.0/2381.5;
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    return 119.0/3674.8;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                        return 83.0/2868.8;
                                    } else {
                                        if ( cl->stats.glue <= 13.5f ) {
                                            return 44.0/3414.9;
                                        } else {
                                            return 42.0/2044.5;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 3.24091696739f ) {
                                    return 198.0/2556.1;
                                } else {
                                    return 176.0/1707.5;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.220541000366f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1211.5f ) {
                                        return 88.0/2590.6;
                                    } else {
                                        return 113.0/1928.8;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 667.0f ) {
                                        return 35.0/2036.4;
                                    } else {
                                        return 86.0/2353.1;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 9853.5f ) {
                            if ( rdb0_last_touched_diff <= 7610.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.710101246834f ) {
                                    return 287.0/2379.5;
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        return 112.0/1902.4;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.882239818573f ) {
                                            return 207.0/2515.5;
                                        } else {
                                            return 282.0/2357.2;
                                        }
                                    }
                                }
                            } else {
                                return 381.0/2755.1;
                            }
                        } else {
                            return 344.0/1443.5;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue <= 9.5f ) {
                if ( cl->stats.glue <= 6.5f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                        if ( cl->size() <= 6.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.217707008123f ) {
                                    return 563.0/1847.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.319914698601f ) {
                                        return 521.0/2273.9;
                                    } else {
                                        if ( cl->size() <= 5.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 433.0/2083.1;
                                            } else {
                                                return 246.0/1884.1;
                                            }
                                        } else {
                                            return 487.0/2342.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.125650465488f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.017060758546f ) {
                                            return 552.0/1035.4;
                                        } else {
                                            if ( cl->size() <= 4.5f ) {
                                                return 594.0/1871.9;
                                            } else {
                                                return 843.0/1878.0;
                                            }
                                        }
                                    } else {
                                        return 371.0/1413.1;
                                    }
                                } else {
                                    return 469.0/2298.3;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( rdb0_last_touched_diff <= 14067.0f ) {
                                    return 521.0/2608.9;
                                } else {
                                    if ( rdb0_last_touched_diff <= 23465.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.130549132824f ) {
                                            return 668.0/1896.3;
                                        } else {
                                            return 437.0/1575.5;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.463544845581f ) {
                                            return 687.0/1786.7;
                                        } else {
                                            return 475.0/1398.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.489872843027f ) {
                                    return 674.0/1157.3;
                                } else {
                                    return 534.0/1102.4;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 14236.5f ) {
                            if ( cl->stats.size_rel <= 0.295157492161f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.140001296997f ) {
                                        return 433.0/1307.5;
                                    } else {
                                        return 325.0/1443.5;
                                    }
                                } else {
                                    return 328.0/1906.4;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 606.0/1289.2;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                        return 475.0/1539.0;
                                    } else {
                                        return 701.0/1756.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.295448839664f ) {
                                if ( rdb0_last_touched_diff <= 49940.0f ) {
                                    if ( cl->size() <= 8.5f ) {
                                        return 470.0/1145.1;
                                    } else {
                                        return 1120.0/1776.5;
                                    }
                                } else {
                                    return 1208.0/1380.6;
                                }
                            } else {
                                return 982.0/2030.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 37133.0f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                    return 686.0/1433.4;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 1098.0/1660.8;
                                    } else {
                                        return 1300.0/1128.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 17411.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0405079238117f ) {
                                        return 614.0/1807.0;
                                    } else {
                                        if ( cl->size() <= 12.5f ) {
                                            return 537.0/1413.1;
                                        } else {
                                            return 851.0/1524.7;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0757628679276f ) {
                                            return 541.0/998.9;
                                        } else {
                                            return 501.0/1187.7;
                                        }
                                    } else {
                                        return 787.0/1250.7;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.557005882263f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 729.0/1003.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.51530611515f ) {
                                        return 701.0/761.4;
                                    } else {
                                        return 797.0/623.3;
                                    }
                                }
                            } else {
                                return 1489.0/1086.2;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 18.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.610896408558f ) {
                                return 442.0/2054.6;
                            } else {
                                return 579.0/2101.3;
                            }
                        } else {
                            return 799.0/2298.3;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.384589314461f ) {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        if ( rdb0_last_touched_diff <= 30358.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.254529476166f ) {
                                    if ( cl->stats.glue_rel_long <= 0.761315226555f ) {
                                        return 472.0/1183.7;
                                    } else {
                                        return 545.0/1011.1;
                                    }
                                } else {
                                    return 952.0/1378.6;
                                }
                            } else {
                                return 647.0/2345.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.786266565323f ) {
                                return 971.0/1520.7;
                            } else {
                                return 1473.0/1134.9;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.813348710537f ) {
                            if ( rdb0_last_touched_diff <= 51611.5f ) {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    return 453.0/1224.3;
                                } else {
                                    return 783.0/1204.0;
                                }
                            } else {
                                return 927.0/521.8;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 107.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.71875f ) {
                                    return 1208.0/1313.6;
                                } else {
                                    return 1007.0/684.2;
                                }
                            } else {
                                return 1193.0/562.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 18.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.809784054756f ) {
                                    return 632.0/913.6;
                                } else {
                                    if ( cl->size() <= 29.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 22025.5f ) {
                                            return 786.0/444.6;
                                        } else {
                                            return 961.0/341.1;
                                        }
                                    } else {
                                        return 821.0/544.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 58626.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.923234701157f ) {
                                        return 756.0/757.3;
                                    } else {
                                        if ( cl->stats.glue <= 12.5f ) {
                                            return 909.0/456.8;
                                        } else {
                                            return 1134.0/373.6;
                                        }
                                    }
                                } else {
                                    return 934.0/306.6;
                                }
                            }
                        } else {
                            return 597.0/1238.5;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 8.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.77067899704f ) {
                                return 1309.0/456.8;
                            } else {
                                if ( cl->stats.glue <= 27.5f ) {
                                    return 1020.0/243.6;
                                } else {
                                    return 1352.0/87.3;
                                }
                            }
                        } else {
                            return 868.0/460.9;
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 10.5f ) {
            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                if ( rdb0_last_touched_diff <= 47644.0f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                    return 714.0/2395.7;
                                } else {
                                    return 746.0/2050.6;
                                }
                            } else {
                                return 636.0/1315.6;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 2522.5f ) {
                                return 182.0/1687.2;
                            } else {
                                return 266.0/1474.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.369253516197f ) {
                            return 555.0/1646.6;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.156531184912f ) {
                                return 951.0/2127.7;
                            } else {
                                return 666.0/1169.4;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 6.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->size() <= 7.5f ) {
                                if ( cl->stats.size_rel <= 0.197908312082f ) {
                                    return 863.0/1303.4;
                                } else {
                                    return 566.0/1045.6;
                                }
                            } else {
                                return 655.0/777.6;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 80534.5f ) {
                                return 809.0/1169.4;
                            } else {
                                if ( cl->stats.size_rel <= 0.177495062351f ) {
                                    return 651.0/775.6;
                                } else {
                                    return 903.0/799.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.768192648888f ) {
                            return 928.0/921.7;
                        } else {
                            return 865.0/464.9;
                        }
                    }
                }
            } else {
                if ( cl->stats.antecedents_glue_long_reds_var <= 0.487112492323f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 108725.0f ) {
                        if ( cl->stats.size_rel <= 0.127011045814f ) {
                            if ( cl->stats.glue_rel_queue <= 0.317292511463f ) {
                                return 495.0/1187.7;
                            } else {
                                return 591.0/1132.9;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 43770.0f ) {
                                return 959.0/1912.5;
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.533318221569f ) {
                                        return 1055.0/1242.5;
                                    } else {
                                        return 649.0/946.1;
                                    }
                                } else {
                                    return 790.0/775.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.533639431f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                if ( rdb0_last_touched_diff <= 211028.5f ) {
                                    return 643.0/797.9;
                                } else {
                                    return 749.0/598.9;
                                }
                            } else {
                                if ( cl->size() <= 8.5f ) {
                                    if ( rdb0_last_touched_diff <= 210041.5f ) {
                                        return 848.0/913.6;
                                    } else {
                                        return 1253.0/804.0;
                                    }
                                } else {
                                    return 818.0/440.6;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.198632270098f ) {
                                return 843.0/625.3;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 174346.0f ) {
                                    return 770.0/556.3;
                                } else {
                                    return 1224.0/481.2;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 62046.0f ) {
                        return 726.0/747.1;
                    } else {
                        if ( cl->stats.size_rel <= 0.376527875662f ) {
                            if ( cl->stats.size_rel <= 0.243003189564f ) {
                                return 916.0/730.9;
                            } else {
                                return 790.0/509.6;
                            }
                        } else {
                            return 1351.0/594.9;
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 49712.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.841489076614f ) {
                    if ( cl->stats.num_overlap_literals <= 86.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0863223150373f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.492307901382f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 18124.0f ) {
                                        return 410.0/1248.6;
                                    } else {
                                        return 498.0/1118.7;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.660619735718f ) {
                                        return 693.0/1352.2;
                                    } else {
                                        return 803.0/1167.4;
                                    }
                                }
                            } else {
                                return 289.0/1640.5;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.846009492874f ) {
                                if ( cl->stats.glue_rel_queue <= 0.486900091171f ) {
                                    return 713.0/1423.2;
                                } else {
                                    if ( cl->size() <= 14.5f ) {
                                        return 808.0/1216.1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            return 647.0/911.6;
                                        } else {
                                            return 889.0/828.4;
                                        }
                                    }
                                }
                            } else {
                                return 1163.0/897.4;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 14205.5f ) {
                            return 698.0/850.7;
                        } else {
                            return 1064.0/588.8;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 4.5f ) {
                        if ( cl->stats.glue <= 9.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.55777776241f ) {
                                return 730.0/639.5;
                            } else {
                                return 1062.0/605.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 60.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.296092927456f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                        return 843.0/854.7;
                                    } else {
                                        return 1453.0/570.5;
                                    }
                                } else {
                                    return 1621.0/560.4;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 14252.0f ) {
                                    return 888.0/324.8;
                                } else {
                                    if ( cl->size() <= 163.5f ) {
                                        if ( rdb0_last_touched_diff <= 27737.0f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                return 1199.0/209.1;
                                            } else {
                                                return 1421.0/339.1;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.24038410187f ) {
                                                if ( cl->stats.num_overlap_literals <= 132.5f ) {
                                                    return 908.0/213.2;
                                                } else {
                                                    return 1623.0/241.6;
                                                }
                                            } else {
                                                return 1775.0/184.8;
                                            }
                                        }
                                    } else {
                                        return 1860.0/127.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 12385.0f ) {
                            return 526.0/1281.1;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.738715291023f ) {
                                return 603.0/925.8;
                            } else {
                                return 1262.0/1019.2;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.563986420631f ) {
                        if ( cl->size() <= 15.5f ) {
                            if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                return 984.0/793.8;
                            } else {
                                return 1039.0/706.5;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 6.75500011444f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 100250.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.394444465637f ) {
                                        return 857.0/629.4;
                                    } else {
                                        return 979.0/588.8;
                                    }
                                } else {
                                    return 1053.0/314.7;
                                }
                            } else {
                                return 1372.0/357.3;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 229.5f ) {
                            if ( cl->stats.glue <= 12.5f ) {
                                return 1234.0/554.3;
                            } else {
                                return 1105.0/282.2;
                            }
                        } else {
                            if ( cl->stats.glue <= 17.5f ) {
                                return 928.0/263.9;
                            } else {
                                return 1446.0/89.3;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.872807979584f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 151718.0f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.303016602993f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0580287948251f ) {
                                    return 1065.0/452.8;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 97628.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.426021277905f ) {
                                            return 628.0/919.7;
                                        } else {
                                            if ( cl->stats.dump_number <= 10.5f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 44.5f ) {
                                                    return 795.0/596.9;
                                                } else {
                                                    return 841.0/385.8;
                                                }
                                            } else {
                                                return 733.0/759.3;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.37175437808f ) {
                                            return 725.0/619.2;
                                        } else {
                                            if ( cl->size() <= 18.5f ) {
                                                return 978.0/548.2;
                                            } else {
                                                return 1001.0/367.5;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 168.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.glue <= 8.5f ) {
                                                return 1063.0/655.8;
                                            } else {
                                                return 1403.0/467.0;
                                            }
                                        } else {
                                            return 1058.0/322.8;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.784453630447f ) {
                                            return 1006.0/310.6;
                                        } else {
                                            return 1028.0/166.5;
                                        }
                                    }
                                } else {
                                    return 1388.0/852.7;
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.22852165997f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.67708337307f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 293753.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.137979626656f ) {
                                            return 886.0/422.3;
                                        } else {
                                            return 847.0/460.9;
                                        }
                                    } else {
                                        return 1686.0/548.2;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.643721342087f ) {
                                        return 1264.0/467.0;
                                    } else {
                                        return 1161.0/278.1;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 272761.5f ) {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.78889477253f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.00499999523f ) {
                                                return 1153.0/288.3;
                                            } else {
                                                return 1651.0/560.4;
                                            }
                                        } else {
                                            return 1774.0/377.6;
                                        }
                                    } else {
                                        return 1264.0/572.5;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.44256311655f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.08163261414f ) {
                                            return 1103.0/148.2;
                                        } else {
                                            return 1320.0/292.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.680018901825f ) {
                                            return 922.0/270.0;
                                        } else {
                                            return 1475.0/335.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 81.5f ) {
                            if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                if ( cl->size() <= 39.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.67708396912f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 1584.0/619.2;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.00597774982f ) {
                                                return 953.0/229.4;
                                            } else {
                                                return 1136.0/245.7;
                                            }
                                        }
                                    } else {
                                        return 1021.0/152.3;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 135234.5f ) {
                                        return 850.0/477.1;
                                    } else {
                                        return 1080.0/284.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 146.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.555884301662f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.508656620979f ) {
                                            return 1331.0/536.0;
                                        } else {
                                            return 1634.0/359.4;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.24704146385f ) {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 55223.0f ) {
                                                    return 977.0/296.4;
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                                        if ( cl->stats.glue <= 10.5f ) {
                                                            return 1358.0/316.7;
                                                        } else {
                                                            if ( cl->stats.antecedents_glue_long_reds_var <= 29.7827777863f ) {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 150457.5f ) {
                                                                    if ( cl->stats.size_rel <= 1.01298260689f ) {
                                                                        return 959.0/196.9;
                                                                    } else {
                                                                        return 990.0/164.5;
                                                                    }
                                                                } else {
                                                                    return 1799.0/168.5;
                                                                }
                                                            } else {
                                                                return 994.0/87.3;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue <= 13.5f ) {
                                                            return 1664.0/469.0;
                                                        } else {
                                                            return 1398.0/231.5;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.664322018623f ) {
                                                    return 1521.0/221.3;
                                                } else {
                                                    return 1765.0/144.2;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 123643.0f ) {
                                                            return 958.0/127.9;
                                                        } else {
                                                            return 1044.0/52.8;
                                                        }
                                                    } else {
                                                        return 1055.0/132.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.819119513035f ) {
                                                        return 934.0/188.8;
                                                    } else {
                                                        return 1069.0/148.2;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.76304090023f ) {
                                                    return 993.0/105.6;
                                                } else {
                                                    return 1206.0/48.7;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.15649521351f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 23.6073970795f ) {
                                            if ( cl->stats.glue <= 15.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 8.64253044128f ) {
                                                    return 1305.0/282.2;
                                                } else {
                                                    return 1089.0/170.5;
                                                }
                                            } else {
                                                return 1910.0/205.1;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                if ( cl->stats.size_rel <= 0.702201843262f ) {
                                                    return 1068.0/44.7;
                                                } else {
                                                    return 1123.0/111.7;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.21155071259f ) {
                                                    return 928.0/176.6;
                                                } else {
                                                    return 1103.0/109.6;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.16594421864f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 123210.0f ) {
                                                return 1163.0/192.9;
                                            } else {
                                                return 1873.0/156.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 352.5f ) {
                                                return 1459.0/46.7;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 973.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 473.5f ) {
                                                        return 949.0/109.6;
                                                    } else {
                                                        if ( cl->stats.glue <= 38.5f ) {
                                                            return 1071.0/95.4;
                                                        } else {
                                                            return 1188.0/65.0;
                                                        }
                                                    }
                                                } else {
                                                    return 1046.0/28.4;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            return 1415.0/521.8;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_short_conf4_cluster0_9(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {

    double rdb_rel_used_for_uip_creation = 0;
    if (cl->stats.used_for_uip_creation > cl->stats.rdb1_used_for_uip_creation) {
        rdb_rel_used_for_uip_creation = 1.0;
    } else {
        rdb_rel_used_for_uip_creation = 0.0;
    }

    double rdb0_avg_confl;
    if (cl->stats.sum_delta_confl_uip1_used == 0) {
        rdb0_avg_confl = 0;
    } else {
        rdb0_avg_confl = ((double)cl->stats.sum_uip1_used)/((double)cl->stats.sum_delta_confl_uip1_used);
    }

    double rdb0_used_per_confl;
    if (sumConflicts-cl->stats.introduced_at_conflict == 0) {
        rdb0_used_per_confl = 0;
    } else {
        rdb0_used_per_confl = ((double)cl->stats.sum_uip1_used)/((double)sumConflicts-(double)cl->stats.introduced_at_conflict);
    }
    if ( cl->stats.rdb1_last_touched_diff <= 30777.5f ) {
        if ( rdb0_last_touched_diff <= 10015.5f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.glue_rel_long <= 0.584074139595f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                        if ( rdb0_last_touched_diff <= 1664.5f ) {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0546325780451f ) {
                                    if ( cl->stats.glue_rel_long <= 0.307365894318f ) {
                                        return 197.0/2377.5;
                                    } else {
                                        return 128.0/2144.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.321775972843f ) {
                                        if ( cl->size() <= 5.5f ) {
                                            if ( cl->size() <= 4.5f ) {
                                                return 137.0/2710.4;
                                            } else {
                                                return 68.0/2081.0;
                                            }
                                        } else {
                                            return 192.0/3268.8;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 655.0f ) {
                                            return 64.0/2012.0;
                                        } else {
                                            return 83.0/1934.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.378971606493f ) {
                                    return 160.0/2424.2;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0727661550045f ) {
                                        return 172.0/1717.6;
                                    } else {
                                        return 139.0/1825.2;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                if ( cl->size() <= 7.5f ) {
                                    if ( rdb0_last_touched_diff <= 7125.0f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0649520978332f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0686822682619f ) {
                                                return 355.0/2917.5;
                                            } else {
                                                return 245.0/2686.1;
                                            }
                                        } else {
                                            if ( cl->size() <= 5.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                    return 145.0/2345.0;
                                                } else {
                                                    return 169.0/2070.9;
                                                }
                                            } else {
                                                return 319.0/3445.4;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 8418.5f ) {
                                            return 213.0/1709.5;
                                        } else {
                                            return 259.0/1701.4;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 6849.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                            return 398.0/2489.1;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.455712676048f ) {
                                                return 255.0/2677.9;
                                            } else {
                                                return 141.0/2032.3;
                                            }
                                        }
                                    } else {
                                        return 413.0/2345.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.271126538515f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.187187701464f ) {
                                            return 311.0/1467.9;
                                        } else {
                                            return 293.0/1543.0;
                                        }
                                    } else {
                                        return 241.0/1666.9;
                                    }
                                } else {
                                    return 192.0/2347.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 5203.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 12.5f ) {
                                if ( rdb0_last_touched_diff <= 1595.5f ) {
                                    if ( rdb0_last_touched_diff <= 748.5f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.10995721817f ) {
                                                return 51.0/2304.4;
                                            } else {
                                                return 27.0/2643.4;
                                            }
                                        } else {
                                            return 109.0/3451.5;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            return 104.0/2107.4;
                                        } else {
                                            return 68.0/2690.1;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.270929306746f ) {
                                            return 133.0/1945.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.26907491684f ) {
                                                return 121.0/3500.2;
                                            } else {
                                                return 122.0/2064.8;
                                            }
                                        }
                                    } else {
                                        return 148.0/1947.0;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.dump_number <= 1.5f ) {
                                        return 97.0/2399.8;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.036364339292f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1890.0f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.038381382823f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.299273252487f ) {
                                                        return 100.0/3644.4;
                                                    } else {
                                                        return 102.0/2554.1;
                                                    }
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 16.5f ) {
                                                        return 53.0/2024.2;
                                                    } else {
                                                        return 25.0/2332.8;
                                                    }
                                                }
                                            } else {
                                                return 147.0/2763.2;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.244405835867f ) {
                                                return 67.0/2245.5;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                    return 15.0/2497.2;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.418343007565f ) {
                                                        if ( rdb0_last_touched_diff <= 2114.5f ) {
                                                            if ( cl->stats.antec_num_total_lits_rel <= 0.138321191072f ) {
                                                                return 46.0/2933.8;
                                                            } else {
                                                                return 19.0/2225.2;
                                                            }
                                                        } else {
                                                            return 64.0/2052.6;
                                                        }
                                                    } else {
                                                        return 67.0/2318.6;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.222540289164f ) {
                                            return 41.0/2060.7;
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 0.356172025204f ) {
                                                return 36.0/2878.9;
                                            } else {
                                                return 28.0/3181.5;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0527797155082f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0221824925393f ) {
                                                return 47.0/2564.2;
                                            } else {
                                                return 73.0/2196.8;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 69.5f ) {
                                                return 66.0/3406.8;
                                            } else {
                                                return 22.0/2113.5;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 7750.0f ) {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0848645344377f ) {
                                        return 175.0/2576.4;
                                    } else {
                                        return 87.0/2008.0;
                                    }
                                } else {
                                    return 214.0/2401.8;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0566410943866f ) {
                                    return 249.0/2405.9;
                                } else {
                                    return 231.0/1833.3;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.332347154617f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 1865.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0417854934931f ) {
                                        return 215.0/3597.7;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                            return 211.0/1835.4;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.230206608772f ) {
                                                return 81.0/2456.6;
                                            } else {
                                                return 34.0/2196.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 16.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                            return 119.0/3146.9;
                                        } else {
                                            return 115.0/2079.0;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                                            return 96.0/1930.8;
                                        } else {
                                            return 28.0/2144.0;
                                        }
                                    }
                                }
                            } else {
                                return 192.0/2097.3;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 4117.0f ) {
                                        return 276.0/1530.8;
                                    } else {
                                        return 385.0/1636.4;
                                    }
                                } else {
                                    if ( cl->size() <= 10.5f ) {
                                        return 204.0/2434.3;
                                    } else {
                                        return 241.0/2107.4;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.671347498894f ) {
                                        return 128.0/2016.1;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                            return 235.0/1867.9;
                                        } else {
                                            return 126.0/1943.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0602877810597f ) {
                                        return 71.0/1914.6;
                                    } else {
                                        return 92.0/1967.3;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 411.0/1640.5;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                        return 365.0/1451.7;
                                    } else {
                                        return 257.0/1798.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 402.0/1880.0;
                                    } else {
                                        return 440.0/1551.1;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 92.5f ) {
                                        if ( rdb0_last_touched_diff <= 3103.0f ) {
                                            return 236.0/3654.5;
                                        } else {
                                            return 252.0/1938.9;
                                        }
                                    } else {
                                        return 225.0/1668.9;
                                    }
                                }
                            }
                        } else {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 235.0/2184.6;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.864172697067f ) {
                                        return 209.0/3603.8;
                                    } else {
                                        return 205.0/2610.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.31679853797f ) {
                                    return 82.0/2351.1;
                                } else {
                                    return 55.0/1997.8;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 12895.0f ) {
                            if ( cl->stats.num_overlap_literals <= 11.5f ) {
                                return 402.0/2334.8;
                            } else {
                                return 518.0/1737.9;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.1434494555f ) {
                                return 679.0/2517.6;
                            } else {
                                return 504.0/1264.9;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.826781690121f ) {
                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                    return 195.0/1904.4;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 12199.5f ) {
                                        return 251.0/2127.7;
                                    } else {
                                        return 466.0/2785.5;
                                    }
                                }
                            } else {
                                return 294.0/1622.2;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.188132718205f ) {
                                return 418.0/2584.6;
                            } else {
                                return 410.0/1329.8;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                return 257.0/3528.6;
                            } else {
                                return 99.0/1908.5;
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 10563.5f ) {
                                    return 221.0/2351.1;
                                } else {
                                    return 240.0/1796.8;
                                }
                            } else {
                                return 241.0/3323.6;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                            return 151.0/3859.6;
                        } else {
                            return 72.0/3303.3;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.872183740139f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.size_rel <= 0.675937891006f ) {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.505102515221f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    return 373.0/1340.0;
                                                } else {
                                                    return 545.0/2497.2;
                                                }
                                            } else {
                                                return 569.0/1809.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.157385632396f ) {
                                                return 413.0/1380.6;
                                            } else {
                                                return 632.0/1675.0;
                                            }
                                        }
                                    } else {
                                        return 534.0/2298.3;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.109435081482f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.527797579765f ) {
                                            if ( rdb0_last_touched_diff <= 21793.0f ) {
                                                return 471.0/1273.0;
                                            } else {
                                                return 515.0/1037.5;
                                            }
                                        } else {
                                            return 553.0/1699.3;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                return 625.0/1987.6;
                                            } else {
                                                return 398.0/1496.3;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 57.5f ) {
                                                return 719.0/1666.9;
                                            } else {
                                                return 415.0/1301.4;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                        return 917.0/2211.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.188012242317f ) {
                                            return 733.0/1007.0;
                                        } else {
                                            return 551.0/1167.4;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.444247096777f ) {
                                        if ( cl->stats.dump_number <= 26.5f ) {
                                            if ( rdb0_last_touched_diff <= 32210.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.127579927444f ) {
                                                    return 413.0/1220.2;
                                                } else {
                                                    return 375.0/1368.4;
                                                }
                                            } else {
                                                return 505.0/1240.5;
                                            }
                                        } else {
                                            return 566.0/1224.3;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 16.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.152755305171f ) {
                                                return 564.0/1553.2;
                                            } else {
                                                return 511.0/1177.6;
                                            }
                                        } else {
                                            return 660.0/1165.4;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 53.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.164439797401f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.624609827995f ) {
                                                return 526.0/1610.0;
                                            } else {
                                                return 511.0/1195.8;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.377098113298f ) {
                                                return 614.0/1589.7;
                                            } else {
                                                return 818.0/1597.8;
                                            }
                                        }
                                    } else {
                                        return 432.0/1382.6;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.618671298027f ) {
                                        return 1005.0/2194.7;
                                    } else {
                                        return 1099.0/1719.7;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->size() <= 17.5f ) {
                                        return 525.0/1348.1;
                                    } else {
                                        return 722.0/1208.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.639779686928f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 103.5f ) {
                                            return 561.0/1114.6;
                                        } else {
                                            return 611.0/844.6;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.438049972057f ) {
                                            return 838.0/741.1;
                                        } else {
                                            return 800.0/846.6;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.697830498219f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 646.0/877.1;
                                } else {
                                    return 757.0/643.6;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 545.0/1275.0;
                                } else {
                                    return 997.0/1758.2;
                                }
                            }
                        } else {
                            if ( cl->size() <= 64.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 17976.5f ) {
                                        return 586.0/936.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16800.0f ) {
                                            return 669.0/779.6;
                                        } else {
                                            return 626.0/852.7;
                                        }
                                    }
                                } else {
                                    return 1143.0/1053.7;
                                }
                            } else {
                                return 792.0/531.9;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 361.0/1413.1;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    return 490.0/2251.6;
                                } else {
                                    return 528.0/2931.7;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.113343402743f ) {
                                return 295.0/2375.4;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                    return 237.0/1999.8;
                                } else {
                                    if ( cl->stats.size_rel <= 0.22501334548f ) {
                                        return 302.0/1474.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.167403832078f ) {
                                            return 240.0/1551.1;
                                        } else {
                                            return 228.0/1666.9;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 4568.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.73001909256f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    return 497.0/1693.3;
                                } else {
                                    if ( rdb0_last_touched_diff <= 11937.5f ) {
                                        return 314.0/2150.1;
                                    } else {
                                        return 347.0/2032.3;
                                    }
                                }
                            } else {
                                return 388.0/1368.4;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.61421751976f ) {
                                return 693.0/2588.6;
                            } else {
                                return 551.0/1358.3;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.antec_num_total_lits_rel <= 0.593454122543f ) {
                    if ( cl->stats.num_overlap_literals <= 72.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.69261789322f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                if ( cl->stats.dump_number <= 2.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 531.0/1013.1;
                                    } else {
                                        return 764.0/761.4;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            return 748.0/2251.6;
                                        } else {
                                            return 658.0/1528.8;
                                        }
                                    } else {
                                        return 711.0/1435.4;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 20054.5f ) {
                                    return 699.0/907.5;
                                } else {
                                    return 721.0/684.2;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 59.5f ) {
                                    return 1045.0/893.3;
                                } else {
                                    return 1675.0/735.0;
                                }
                            } else {
                                return 1010.0/1721.7;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 3.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 14267.0f ) {
                                return 1067.0/420.3;
                            } else {
                                return 992.0/164.5;
                            }
                        } else {
                            return 565.0/925.8;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->size() <= 14.5f ) {
                            return 706.0/858.8;
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.04422473907f ) {
                                return 1350.0/1260.8;
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 9.01350307465f ) {
                                    return 868.0/688.3;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                        return 946.0/233.5;
                                    } else {
                                        return 1399.0/481.2;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 7.96958255768f ) {
                            if ( cl->size() <= 18.5f ) {
                                return 773.0/629.4;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.09528648853f ) {
                                    return 856.0/373.6;
                                } else {
                                    return 1089.0/326.9;
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 15.5f ) {
                                return 1629.0/574.6;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 1.23824095726f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.14435184002f ) {
                                        return 1013.0/259.9;
                                    } else {
                                        return 1554.0/235.5;
                                    }
                                } else {
                                    return 1629.0/190.8;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
            if ( cl->size() <= 10.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 102036.5f ) {
                    if ( cl->size() <= 8.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            return 760.0/2137.9;
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->size() <= 4.5f ) {
                                    return 738.0/1626.3;
                                } else {
                                    if ( rdb0_last_touched_diff <= 52413.0f ) {
                                        return 923.0/1829.3;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0545369908214f ) {
                                                return 823.0/1126.8;
                                            } else {
                                                return 606.0/1049.7;
                                            }
                                        } else {
                                            return 676.0/844.6;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 51254.5f ) {
                                        return 880.0/1638.4;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.227837383747f ) {
                                            return 1160.0/1409.0;
                                        } else {
                                            return 754.0/1031.4;
                                        }
                                    }
                                } else {
                                    return 1318.0/1498.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.584513902664f ) {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 1160.0/1770.4;
                            } else {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    return 1376.0/1222.2;
                                } else {
                                    return 1059.0/1319.7;
                                }
                            }
                        } else {
                            return 796.0/471.0;
                        }
                    }
                } else {
                    if ( cl->size() <= 8.5f ) {
                        if ( cl->size() <= 5.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                return 974.0/1285.2;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0315138697624f ) {
                                    return 936.0/657.8;
                                } else {
                                    return 1162.0/1258.8;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.229905098677f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0269904565066f ) {
                                    return 708.0/745.1;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 875.0/820.2;
                                    } else {
                                        return 1289.0/950.2;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.182369843125f ) {
                                    return 893.0/460.9;
                                } else {
                                    return 1015.0/759.3;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.349977910519f ) {
                            if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                return 1419.0/940.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.604432821274f ) {
                                    return 917.0/454.8;
                                } else {
                                    return 941.0/414.2;
                                }
                            }
                        } else {
                            return 1103.0/456.8;
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.904610335827f ) {
                    if ( cl->stats.glue_rel_long <= 0.613087415695f ) {
                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 48459.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.175515204668f ) {
                                    return 605.0/925.8;
                                } else {
                                    return 663.0/860.8;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 78950.0f ) {
                                    return 1226.0/1208.0;
                                } else {
                                    return 800.0/538.0;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.383284628391f ) {
                                if ( rdb0_last_touched_diff <= 162145.0f ) {
                                    if ( rdb0_last_touched_diff <= 64519.0f ) {
                                        return 709.0/925.8;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0830175131559f ) {
                                            return 915.0/952.2;
                                        } else {
                                            return 1018.0/574.6;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 267393.5f ) {
                                        return 837.0/487.3;
                                    } else {
                                        return 1091.0/381.7;
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 185946.0f ) {
                                        if ( cl->stats.glue <= 8.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.4561111927f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.251417815685f ) {
                                                    return 730.0/643.6;
                                                } else {
                                                    return 806.0/460.9;
                                                }
                                            } else {
                                                return 707.0/653.8;
                                            }
                                        } else {
                                            return 1225.0/692.3;
                                        }
                                    } else {
                                        return 1086.0/261.9;
                                    }
                                } else {
                                    if ( cl->size() <= 44.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0799220502377f ) {
                                            return 1067.0/266.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.762868046761f ) {
                                                if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                                    return 1039.0/460.9;
                                                } else {
                                                    return 1116.0/389.8;
                                                }
                                            } else {
                                                return 1141.0/282.2;
                                            }
                                        }
                                    } else {
                                        return 1364.0/842.6;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 96273.0f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 73.5f ) {
                                    return 732.0/1033.4;
                                } else {
                                    return 751.0/570.5;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                    if ( rdb0_last_touched_diff <= 58345.5f ) {
                                        return 1125.0/1189.7;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.670748949051f ) {
                                            return 1077.0/887.2;
                                        } else {
                                            return 1142.0/639.5;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 353.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.350569546223f ) {
                                                return 1259.0/605.0;
                                            } else {
                                                return 1144.0/903.5;
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 8.5f ) {
                                                return 986.0/613.1;
                                            } else {
                                                if ( cl->stats.dump_number <= 8.5f ) {
                                                    return 1574.0/491.3;
                                                } else {
                                                    return 930.0/548.2;
                                                }
                                            }
                                        }
                                    } else {
                                        return 1046.0/288.3;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.433740317822f ) {
                                if ( rdb0_last_touched_diff <= 170163.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.23799110949f ) {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            return 849.0/467.0;
                                        } else {
                                            return 1339.0/400.0;
                                        }
                                    } else {
                                        return 1396.0/751.2;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 1.55328798294f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 300563.5f ) {
                                            if ( cl->stats.dump_number <= 25.5f ) {
                                                return 1018.0/199.0;
                                            } else {
                                                return 953.0/318.8;
                                            }
                                        } else {
                                            return 1694.0/286.3;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.776577591896f ) {
                                            return 1758.0/558.3;
                                        } else {
                                            return 1056.0/215.2;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.481090903282f ) {
                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                        return 1255.0/286.3;
                                    } else {
                                        return 1000.0/166.5;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( rdb0_last_touched_diff <= 238965.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.836337268353f ) {
                                                return 1001.0/375.6;
                                            } else {
                                                return 1089.0/351.2;
                                            }
                                        } else {
                                            return 1468.0/337.0;
                                        }
                                    } else {
                                        return 1395.0/278.1;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 118.5f ) {
                        if ( cl->stats.size_rel <= 0.925175786018f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->size() <= 15.5f ) {
                                    return 925.0/726.8;
                                } else {
                                    return 1408.0/822.3;
                                }
                            } else {
                                if ( cl->size() <= 42.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 64.5f ) {
                                            return 1134.0/479.1;
                                        } else {
                                            return 980.0/207.1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.05409789085f ) {
                                            if ( cl->stats.dump_number <= 28.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 3.56944441795f ) {
                                                    return 1107.0/393.9;
                                                } else {
                                                    return 984.0/225.4;
                                                }
                                            } else {
                                                return 952.0/217.2;
                                            }
                                        } else {
                                            if ( cl->size() <= 21.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 28.5f ) {
                                                    return 981.0/192.9;
                                                } else {
                                                    return 1057.0/182.7;
                                                }
                                            } else {
                                                return 930.0/231.5;
                                            }
                                        }
                                    }
                                } else {
                                    return 822.0/446.7;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 85727.5f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 1770.0/462.9;
                                    } else {
                                        return 1310.0/637.5;
                                    }
                                } else {
                                    if ( cl->size() <= 42.5f ) {
                                        if ( cl->stats.glue_rel_long <= 1.2281332016f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                return 1085.0/140.1;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.521857380867f ) {
                                                    return 1371.0/349.2;
                                                } else {
                                                    return 1144.0/190.8;
                                                }
                                            }
                                        } else {
                                            return 1597.0/166.5;
                                        }
                                    } else {
                                        return 1004.0/324.8;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 174236.0f ) {
                                    return 956.0/142.1;
                                } else {
                                    return 1550.0/109.6;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 76259.5f ) {
                                return 1322.0/562.4;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.86094164848f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 192.5f ) {
                                        return 1485.0/274.1;
                                    } else {
                                        return 1025.0/117.8;
                                    }
                                } else {
                                    return 942.0/284.2;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 456.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.13406348228f ) {
                                        return 1114.0/422.3;
                                    } else {
                                        return 990.0/196.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                        return 912.0/286.3;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 60859.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.20020878315f ) {
                                                return 1161.0/304.5;
                                            } else {
                                                return 1270.0/227.4;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.size_rel <= 1.95089483261f ) {
                                                        if ( cl->stats.glue <= 17.5f ) {
                                                            if ( cl->stats.dump_number <= 24.5f ) {
                                                                if ( cl->stats.glue_rel_queue <= 1.09156417847f ) {
                                                                    return 1050.0/180.7;
                                                                } else {
                                                                    return 1548.0/146.2;
                                                                }
                                                            } else {
                                                                return 955.0/203.0;
                                                            }
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.382075071335f ) {
                                                                return 1228.0/152.3;
                                                            } else {
                                                                if ( rdb0_last_touched_diff <= 123139.0f ) {
                                                                    return 1093.0/115.7;
                                                                } else {
                                                                    return 1460.0/79.2;
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        return 1107.0/62.9;
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 15.5f ) {
                                                        return 1873.0/233.5;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 252313.5f ) {
                                                            return 943.0/231.5;
                                                        } else {
                                                            return 1438.0/241.6;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.723833799362f ) {
                                                    return 1326.0/172.6;
                                                } else {
                                                    if ( cl->stats.size_rel <= 1.16981434822f ) {
                                                        return 1047.0/107.6;
                                                    } else {
                                                        return 2023.0/60.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 1.09720885754f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            return 1249.0/97.5;
                                        } else {
                                            return 1649.0/233.5;
                                        }
                                    } else {
                                        return 1044.0/217.2;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                if ( cl->size() <= 120.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 2.49193191528f ) {
                                                        return 1270.0/67.0;
                                                    } else {
                                                        return 1003.0/113.7;
                                                    }
                                                } else {
                                                    return 1776.0/50.8;
                                                }
                                            } else {
                                                return 1707.0/136.0;
                                            }
                                        } else {
                                            return 951.0/142.1;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.42892372608f ) {
                                            return 977.0/58.9;
                                        } else {
                                            return 1183.0/36.5;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->size() <= 10.5f ) {
                if ( cl->stats.glue_rel_queue <= 0.646805286407f ) {
                    if ( rdb0_last_touched_diff <= 3516.0f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.0984998717904f ) {
                            return 240.0/1689.2;
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.17766058445f ) {
                                return 191.0/1941.0;
                            } else {
                                return 232.0/1977.5;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                            return 405.0/1455.7;
                        } else {
                            return 507.0/1514.6;
                        }
                    }
                } else {
                    return 494.0/2079.0;
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.800890564919f ) {
                    if ( cl->stats.size_rel <= 0.72078537941f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            return 846.0/2129.8;
                        } else {
                            return 440.0/3222.1;
                        }
                    } else {
                        return 537.0/1819.1;
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 46.5f ) {
                        return 697.0/2517.6;
                    } else {
                        return 683.0/1622.2;
                    }
                }
            }
        }
    }
}

static bool should_keep_short_conf4_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_short_conf4_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_short_conf4_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
