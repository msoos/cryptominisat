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

static double estimator_should_keep_long_conf3_cluster0_0(
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
    if ( cl->stats.sum_delta_confl_uip1_used <= 5109.0f ) {
        if ( cl->stats.glue_rel_queue <= 0.796319246292f ) {
            if ( cl->stats.dump_number <= 6.5f ) {
                if ( cl->stats.size_rel <= 0.774254918098f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.209185779095f ) {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0609665587544f ) {
                                        return 84.0/365.9;
                                    } else {
                                        return 145.0/451.9;
                                    }
                                } else {
                                    return 71.0/433.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 88.0/290.0;
                                } else {
                                    return 121.0/224.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.613766014576f ) {
                                return 107.0/244.0;
                            } else {
                                return 141.0/240.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 157.0/339.9;
                            } else {
                                return 181.0/244.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                return 315.0/174.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 50.5f ) {
                                    return 184.0/274.0;
                                } else {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 143.0/156.0;
                                    } else {
                                        return 160.0/118.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 22264.0f ) {
                        return 290.0/222.0;
                    } else {
                        return 360.0/150.0;
                    }
                }
            } else {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    return 79.0/383.9;
                } else {
                    if ( rdb0_last_touched_diff <= 192636.5f ) {
                        if ( cl->stats.size_rel <= 0.732147693634f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 458.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 21.5f ) {
                                    return 247.0/262.0;
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 181.0/88.0;
                                        } else {
                                            return 228.0/228.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 188.0/92.0;
                                        } else {
                                            return 191.0/56.0;
                                        }
                                    }
                                }
                            } else {
                                return 192.0/246.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                return 389.0/60.0;
                            } else {
                                return 246.0/114.0;
                            }
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            return 304.0/226.0;
                        } else {
                            if ( cl->size() <= 48.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 146.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.729631245136f ) {
                                        if ( cl->stats.dump_number <= 40.5f ) {
                                            if ( rdb0_last_touched_diff <= 288577.0f ) {
                                                return 233.0/82.0;
                                            } else {
                                                return 156.0/90.0;
                                            }
                                        } else {
                                            return 193.0/24.0;
                                        }
                                    } else {
                                        return 202.0/24.0;
                                    }
                                } else {
                                    return 252.0/40.0;
                                }
                            } else {
                                return 193.0/16.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 20.5f ) {
                if ( cl->stats.dump_number <= 6.5f ) {
                    if ( cl->stats.dump_number <= 2.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            return 212.0/236.0;
                        } else {
                            return 162.0/272.0;
                        }
                    } else {
                        if ( cl->size() <= 12.5f ) {
                            return 144.0/218.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                return 251.0/58.0;
                            } else {
                                return 191.0/114.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 112595.0f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0932885408401f ) {
                            return 153.0/110.0;
                        } else {
                            return 191.0/84.0;
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.379029721022f ) {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->size() <= 15.5f ) {
                                    return 230.0/124.0;
                                } else {
                                    return 338.0/66.0;
                                }
                            } else {
                                return 276.0/36.0;
                            }
                        } else {
                            return 250.0/24.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.493493080139f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 169.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 39203.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 74.5f ) {
                                return 126.0/160.0;
                            } else {
                                return 208.0/142.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 1.04748809338f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.269761621952f ) {
                                    return 163.0/62.0;
                                } else {
                                    return 176.0/78.0;
                                }
                            } else {
                                return 258.0/42.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.389685600996f ) {
                            return 199.0/94.0;
                        } else {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 189.0/64.0;
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.63599300385f ) {
                                        return 195.0/40.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 1.82767105103f ) {
                                            return 339.0/14.0;
                                        } else {
                                            return 200.0/24.0;
                                        }
                                    }
                                } else {
                                    return 218.0/42.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 109.5f ) {
                        if ( cl->size() <= 14.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 6.69261789322f ) {
                                return 302.0/162.0;
                            } else {
                                return 345.0/56.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 9956.0f ) {
                                if ( cl->stats.glue_rel_queue <= 1.10764026642f ) {
                                    return 225.0/130.0;
                                } else {
                                    return 192.0/26.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.876058459282f ) {
                                    if ( cl->size() <= 24.5f ) {
                                        return 252.0/58.0;
                                    } else {
                                        return 242.0/92.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 1.26888155937f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 1.12083148956f ) {
                                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                                if ( cl->size() <= 32.5f ) {
                                                    if ( cl->stats.glue <= 10.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.488742381334f ) {
                                                            return 194.0/28.0;
                                                        } else {
                                                            return 230.0/76.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.size_rel <= 0.738726377487f ) {
                                                            return 198.0/48.0;
                                                        } else {
                                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                                    return 240.0/12.0;
                                                                } else {
                                                                    return 361.0/86.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.rdb1_last_touched_diff <= 96183.0f ) {
                                                                    return 226.0/26.0;
                                                                } else {
                                                                    return 294.0/16.0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.0363972187f ) {
                                                        return 290.0/54.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 1.48268842697f ) {
                                                            if ( cl->stats.glue_rel_queue <= 1.15154337883f ) {
                                                                return 277.0/20.0;
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals <= 62.5f ) {
                                                                    return 209.0/2.0;
                                                                } else {
                                                                    return 211.0/8.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 270.0/38.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 164341.0f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                        return 321.0/74.0;
                                                    } else {
                                                        return 291.0/108.0;
                                                    }
                                                } else {
                                                    return 313.0/44.0;
                                                }
                                            }
                                        } else {
                                            return 213.0/10.0;
                                        }
                                    } else {
                                        return 306.0/84.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 12.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 2.1483464241f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 1.06976032257f ) {
                                    return 317.0/34.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.988587975502f ) {
                                        return 261.0/100.0;
                                    } else {
                                        return 319.0/34.0;
                                    }
                                }
                            } else {
                                return 219.0/76.0;
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    if ( rdb0_last_touched_diff <= 40282.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 417.5f ) {
                                            if ( cl->stats.glue <= 20.5f ) {
                                                return 254.0/22.0;
                                            } else {
                                                return 175.0/56.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 729.0f ) {
                                                return 203.0/2.0;
                                            } else {
                                                return 408.0/20.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 234.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 1.21928203106f ) {
                                                if ( cl->stats.glue_rel_queue <= 1.13122868538f ) {
                                                    return 251.0/14.0;
                                                } else {
                                                    return 1;
                                                }
                                            } else {
                                                return 218.0/24.0;
                                            }
                                        } else {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 174.951751709f ) {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                                    return 1;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4.5f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 611.5f ) {
                                                            return 195.0/6.0;
                                                        } else {
                                                            return 270.0/4.0;
                                                        }
                                                    } else {
                                                        return 1;
                                                    }
                                                }
                                            } else {
                                                return 237.0/12.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 313.0/60.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 286341.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 330.0f ) {
                                            return 405.0/28.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.7554192543f ) {
                                                if ( cl->stats.size_rel <= 0.926457285881f ) {
                                                    return 198.0/28.0;
                                                } else {
                                                    return 330.0/110.0;
                                                }
                                            } else {
                                                return 352.0/28.0;
                                            }
                                        }
                                    } else {
                                        return 205.0/60.0;
                                    }
                                } else {
                                    if ( cl->size() <= 101.5f ) {
                                        if ( cl->size() <= 37.5f ) {
                                            return 188.0/4.0;
                                        } else {
                                            return 1;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.20842242241f ) {
                                            return 201.0/20.0;
                                        } else {
                                            return 263.0/12.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 37025.0f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 900496.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 10814.5f ) {
                                            return 96.0/299.9;
                                        } else {
                                            return 96.0/256.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 7.5f ) {
                                            return 163.0/469.9;
                                        } else {
                                            return 222.0/220.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 81.0/517.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0750736072659f ) {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 79.0/268.0;
                                                } else {
                                                    return 74.0/298.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                    return 58.0/375.9;
                                                } else {
                                                    return 66.0/294.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 117.0/294.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 185175.0f ) {
                                    return 68.0/407.9;
                                } else {
                                    return 50.0/491.9;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 13.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 133008.0f ) {
                                    if ( cl->stats.size_rel <= 0.740744709969f ) {
                                        if ( cl->stats.glue_rel_long <= 0.83220988512f ) {
                                            if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                                return 123.0/202.0;
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 98.0/200.0;
                                                } else {
                                                    return 114.0/301.9;
                                                }
                                            }
                                        } else {
                                            return 176.0/200.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 9635.5f ) {
                                            return 148.0/218.0;
                                        } else {
                                            return 255.0/148.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.566168665886f ) {
                                        return 114.0/567.9;
                                    } else {
                                        if ( cl->size() <= 13.5f ) {
                                            return 84.0/274.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 113.0/248.0;
                                            } else {
                                                return 130.0/254.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 21.5f ) {
                                    return 180.0/194.0;
                                } else {
                                    return 204.0/100.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.659592032433f ) {
                            if ( cl->stats.sum_uip1_used <= 90.5f ) {
                                if ( cl->stats.dump_number <= 15.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0865519791842f ) {
                                        if ( cl->stats.glue_rel_long <= 0.371497750282f ) {
                                            return 42.0/401.9;
                                        } else {
                                            return 64.0/274.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 10.5f ) {
                                            return 38.0/581.9;
                                        } else {
                                            return 70.0/457.9;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 20.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0622317045927f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                return 103.0/284.0;
                                            } else {
                                                return 131.0/208.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 8837.0f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                    return 95.0/299.9;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.15729431808f ) {
                                                        return 83.0/313.9;
                                                    } else {
                                                        return 62.0/389.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 42.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 15352.5f ) {
                                                        return 120.0/222.0;
                                                    } else {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                                            return 79.0/278.0;
                                                        } else {
                                                            return 106.0/216.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 9834785.0f ) {
                                                        return 56.0/337.9;
                                                    } else {
                                                        return 89.0/266.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 125.0/192.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.29870557785f ) {
                                    if ( rdb0_last_touched_diff <= 24649.0f ) {
                                        if ( cl->stats.glue_rel_long <= 0.213819354773f ) {
                                            return 57.0/475.9;
                                        } else {
                                            return 38.0/449.9;
                                        }
                                    } else {
                                        return 82.0/391.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( rdb0_last_touched_diff <= 11613.0f ) {
                                            return 14.0/531.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0230668969452f ) {
                                                return 47.0/315.9;
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 172.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0840163975954f ) {
                                                        return 64.0/407.9;
                                                    } else {
                                                        return 32.0/463.9;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.10924448818f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0972335487604f ) {
                                                            return 21.0/401.9;
                                                        } else {
                                                            return 13.0/439.9;
                                                        }
                                                    } else {
                                                        return 23.0/331.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 22103.0f ) {
                                            return 26.0/375.9;
                                        } else {
                                            return 43.0/311.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                return 194.0/238.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 55.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 62.5f ) {
                                        return 169.0/465.9;
                                    } else {
                                        return 97.0/561.9;
                                    }
                                } else {
                                    return 47.0/351.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                        if ( rdb0_last_touched_diff <= 71181.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.618683278561f ) {
                                if ( cl->stats.sum_uip1_used <= 26.5f ) {
                                    return 202.0/353.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 11051288.0f ) {
                                        return 92.0/407.9;
                                    } else {
                                        return 63.0/447.9;
                                    }
                                }
                            } else {
                                return 174.0/377.9;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.583722710609f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0278436206281f ) {
                                    return 143.0/333.9;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                        return 117.0/288.0;
                                    } else {
                                        return 149.0/194.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                    return 159.0/120.0;
                                } else {
                                    return 106.0/210.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.58714401722f ) {
                            if ( rdb0_last_touched_diff <= 94663.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 2437454.5f ) {
                                    if ( cl->size() <= 15.5f ) {
                                        return 148.0/270.0;
                                    } else {
                                        return 137.0/194.0;
                                    }
                                } else {
                                    return 95.0/268.0;
                                }
                            } else {
                                return 170.0/186.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.122180774808f ) {
                                return 202.0/102.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                    if ( cl->stats.glue <= 11.5f ) {
                                        return 199.0/164.0;
                                    } else {
                                        return 228.0/96.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 52.5f ) {
                                        return 113.0/200.0;
                                    } else {
                                        return 135.0/170.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                    if ( rdb0_last_touched_diff <= 97128.5f ) {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                return 199.0/234.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.245435848832f ) {
                                    return 200.0/284.0;
                                } else {
                                    return 132.0/327.9;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.638719379902f ) {
                                if ( cl->stats.glue_rel_long <= 0.446063786745f ) {
                                    return 100.0/238.0;
                                } else {
                                    return 153.0/166.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 236.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 25488.0f ) {
                                        return 179.0/86.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 142090.5f ) {
                                            return 230.0/242.0;
                                        } else {
                                            return 194.0/96.0;
                                        }
                                    }
                                } else {
                                    return 211.0/66.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 9.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.132076352835f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.46192035079f ) {
                                        return 143.0/128.0;
                                    } else {
                                        return 135.0/162.0;
                                    }
                                } else {
                                    return 232.0/148.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 39.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.696819186211f ) {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            return 149.0/162.0;
                                        } else {
                                            return 224.0/152.0;
                                        }
                                    } else {
                                        return 325.0/170.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 435072.5f ) {
                                        return 297.0/66.0;
                                    } else {
                                        return 163.0/92.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.650806844234f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 45383.5f ) {
                                    return 281.0/168.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                        return 276.0/36.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 206875.0f ) {
                                            return 190.0/94.0;
                                        } else {
                                            return 173.0/66.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.02295982838f ) {
                                    return 179.0/44.0;
                                } else {
                                    return 295.0/38.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 13.5f ) {
                        if ( cl->stats.size_rel <= 0.780643761158f ) {
                            if ( cl->stats.sum_uip1_used <= 48.5f ) {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.628641963005f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 97.0/525.9;
                                        } else {
                                            return 49.0/407.9;
                                        }
                                    } else {
                                        return 74.0/250.0;
                                    }
                                } else {
                                    return 117.0/200.0;
                                }
                            } else {
                                return 48.0/739.9;
                            }
                        } else {
                            return 109.0/188.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                            if ( rdb0_last_touched_diff <= 230919.0f ) {
                                if ( cl->stats.sum_uip1_used <= 47.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2008691.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                            return 94.0/244.0;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                                return 137.0/240.0;
                                            } else {
                                                return 182.0/230.0;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 119100.0f ) {
                                            return 124.0/154.0;
                                        } else {
                                            return 156.0/134.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.325983047485f ) {
                                        return 111.0/290.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.527220606804f ) {
                                            return 80.0/533.9;
                                        } else {
                                            return 110.0/471.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0259701516479f ) {
                                    return 173.0/142.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.129552930593f ) {
                                            return 112.0/212.0;
                                        } else {
                                            return 129.0/172.0;
                                        }
                                    } else {
                                        return 157.0/138.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 140170.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.854650974274f ) {
                                    return 229.0/333.9;
                                } else {
                                    return 92.0/214.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 29.5f ) {
                                    return 305.0/192.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.272336453199f ) {
                                        return 174.0/114.0;
                                    } else {
                                        return 225.0/296.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_uip1_used <= 21.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 25636.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 756035.0f ) {
                                if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                    return 96.0/371.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0954611599445f ) {
                                        return 75.0/469.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0937276631594f ) {
                                            return 33.0/403.9;
                                        } else {
                                            return 61.0/451.9;
                                        }
                                    }
                                }
                            } else {
                                return 79.0/238.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                return 113.0/467.9;
                            } else {
                                return 175.0/367.9;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 127274.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.450631767511f ) {
                                    return 26.0/363.9;
                                } else {
                                    return 15.0/375.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0890499204397f ) {
                                    return 50.0/377.9;
                                } else {
                                    return 21.0/339.9;
                                }
                            }
                        } else {
                            return 79.0/543.9;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.57243168354f ) {
                        if ( cl->size() <= 17.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                return 133.0/353.9;
                            } else {
                                return 70.0/339.9;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 2.5f ) {
                                return 41.0/373.9;
                            } else {
                                return 111.0/497.9;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 5.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.399399459362f ) {
                                if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                    return 110.0/222.0;
                                } else {
                                    return 149.0/168.0;
                                }
                            } else {
                                return 216.0/208.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.061486043036f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.0930735468864f ) {
                                    return 74.0/266.0;
                                } else {
                                    return 49.0/325.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.59499502182f ) {
                                        if ( rdb0_last_touched_diff <= 5506.0f ) {
                                            if ( cl->size() <= 16.5f ) {
                                                return 60.0/315.9;
                                            } else {
                                                return 112.0/299.9;
                                            }
                                        } else {
                                            return 99.0/258.0;
                                        }
                                    } else {
                                        return 139.0/276.0;
                                    }
                                } else {
                                    return 164.0/274.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 6728.5f ) {
                    if ( cl->stats.sum_uip1_used <= 88.5f ) {
                        if ( cl->stats.sum_uip1_used <= 28.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->size() <= 10.5f ) {
                                    return 71.0/487.9;
                                } else {
                                    return 71.0/288.0;
                                }
                            } else {
                                return 42.0/551.9;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 4402590.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 44.5f ) {
                                            return 28.0/411.9;
                                        } else {
                                            return 17.0/381.9;
                                        }
                                    } else {
                                        return 37.0/333.9;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.332967996597f ) {
                                        if ( cl->stats.dump_number <= 2.5f ) {
                                            return 17.0/369.9;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                                return 13.0/569.9;
                                            } else {
                                                return 13.0/413.9;
                                            }
                                        }
                                    } else {
                                        return 30.0/385.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 133.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 28.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.247934564948f ) {
                                            return 62.0/347.9;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 1152.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.161821559072f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.117649883032f ) {
                                                        return 39.0/387.9;
                                                    } else {
                                                        return 29.0/357.9;
                                                    }
                                                } else {
                                                    return 22.0/607.9;
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 34.5f ) {
                                                    return 20.0/419.9;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 5926.5f ) {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1351888.5f ) {
                                                            return 21.0/495.9;
                                                        } else {
                                                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                                                return 50.0/537.9;
                                                            } else {
                                                                return 61.0/441.9;
                                                            }
                                                        }
                                                    } else {
                                                        return 52.0/327.9;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 9.0/589.9;
                                    }
                                } else {
                                    return 84.0/605.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0192497074604f ) {
                            if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.glue <= 4.5f ) {
                                    return 19.0/435.9;
                                } else {
                                    return 44.0/391.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 3729.0f ) {
                                    if ( cl->size() <= 10.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1921.0f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 347.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                    return 11.0/455.9;
                                                } else {
                                                    return 6.0/495.9;
                                                }
                                            } else {
                                                return 5.0/855.9;
                                            }
                                        } else {
                                            return 20.0/493.9;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                                            return 24.0/437.9;
                                        } else {
                                            return 13.0/475.9;
                                        }
                                    }
                                } else {
                                    return 34.0/401.9;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 2756.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        return 16.0/633.9;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 16247566.0f ) {
                                            if ( cl->stats.used_for_uip_creation <= 21.5f ) {
                                                return 3.0/443.9;
                                            } else {
                                                return 9.0/427.9;
                                            }
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                                                return 4.0/475.9;
                                            } else {
                                                return 0.0/1169.8;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.26339712739f ) {
                                        if ( rdb0_last_touched_diff <= 558.5f ) {
                                            return 13.0/379.9;
                                        } else {
                                            return 30.0/385.9;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 186.5f ) {
                                            if ( cl->stats.size_rel <= 0.196830838919f ) {
                                                return 7.0/391.9;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                                    return 32.0/363.9;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 18.5f ) {
                                                        return 20.0/357.9;
                                                    } else {
                                                        return 12.0/531.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 10.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0953270792961f ) {
                                                    return 14.0/663.9;
                                                } else {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                                        return 7.0/719.9;
                                                    } else {
                                                        return 0.0/723.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.188046216965f ) {
                                                    return 16.0/389.9;
                                                } else {
                                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                        return 10.0/439.9;
                                                    } else {
                                                        return 3.0/477.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                    return 46.0/595.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.434012711048f ) {
                                            if ( cl->stats.glue_rel_long <= 0.369910925627f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                                    return 10.0/417.9;
                                                } else {
                                                    return 8.0/651.9;
                                                }
                                            } else {
                                                return 19.0/369.9;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.544889032841f ) {
                                                return 1.0/509.9;
                                            } else {
                                                return 11.0/723.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.186117261648f ) {
                                            return 52.0/659.9;
                                        } else {
                                            return 13.0/587.9;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 33026524.0f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.330195069313f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0986350476742f ) {
                                            if ( cl->stats.dump_number <= 29.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.102483928204f ) {
                                                    return 42.0/379.9;
                                                } else {
                                                    return 24.0/381.9;
                                                }
                                            } else {
                                                return 75.0/353.9;
                                            }
                                        } else {
                                            return 64.0/294.0;
                                        }
                                    } else {
                                        return 99.0/463.9;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 26.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                            if ( cl->stats.sum_uip1_used <= 40.5f ) {
                                                return 57.0/561.9;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0987697392702f ) {
                                                    return 36.0/647.9;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                                                        return 22.0/591.9;
                                                    } else {
                                                        return 1.0/397.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 15389.5f ) {
                                                return 44.0/599.9;
                                            } else {
                                                return 46.0/423.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            return 33.0/361.9;
                                        } else {
                                            return 62.0/415.9;
                                        }
                                    }
                                }
                            } else {
                                return 88.0/529.9;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3325.5f ) {
                                return 73.0/559.9;
                            } else {
                                return 100.0/359.9;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.191257283092f ) {
                            return 68.0/701.9;
                        } else {
                            if ( cl->stats.dump_number <= 94.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 8.0/561.9;
                                } else {
                                    return 22.0/583.9;
                                }
                            } else {
                                return 38.0/479.9;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_1(
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
    if ( cl->stats.sum_delta_confl_uip1_used <= 6406.0f ) {
        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.glue_rel_queue <= 0.875571548939f ) {
                if ( cl->size() <= 9.5f ) {
                    if ( rdb0_last_touched_diff <= 11300.0f ) {
                        return 35.0/347.9;
                    } else {
                        if ( cl->stats.glue <= 4.5f ) {
                            return 82.0/503.9;
                        } else {
                            return 163.0/459.9;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.534879207611f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 8643.5f ) {
                            return 97.0/258.0;
                        } else {
                            return 118.0/184.0;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.90037304163f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.13227891922f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.204900726676f ) {
                                    return 110.0/190.0;
                                } else {
                                    return 130.0/176.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 55.0f ) {
                                    return 208.0/156.0;
                                } else {
                                    return 143.0/220.0;
                                }
                            }
                        } else {
                            return 222.0/130.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_overlap_literals_rel <= 0.302474498749f ) {
                    if ( cl->stats.num_overlap_literals <= 24.5f ) {
                        if ( cl->size() <= 16.5f ) {
                            return 130.0/194.0;
                        } else {
                            return 163.0/120.0;
                        }
                    } else {
                        return 239.0/72.0;
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 17.0816326141f ) {
                        if ( rdb0_last_touched_diff <= 22390.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.07251369953f ) {
                                return 205.0/144.0;
                            } else {
                                return 207.0/50.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.766872346401f ) {
                                return 170.0/54.0;
                            } else {
                                return 245.0/24.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 15342.5f ) {
                            return 231.0/32.0;
                        } else {
                            if ( cl->stats.glue <= 19.5f ) {
                                return 212.0/20.0;
                            } else {
                                return 305.0/4.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.775414466858f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 38624.0f ) {
                    if ( cl->size() <= 17.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.130084350705f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.10253752023f ) {
                                return 115.0/234.0;
                            } else {
                                return 70.0/331.9;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                return 144.0/327.9;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 19656.5f ) {
                                    return 133.0/188.0;
                                } else {
                                    return 153.0/156.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.199782326818f ) {
                                return 189.0/70.0;
                            } else {
                                return 163.0/120.0;
                            }
                        } else {
                            return 185.0/218.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.532158911228f ) {
                        if ( cl->stats.glue_rel_queue <= 0.200806453824f ) {
                            return 136.0/164.0;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.107749700546f ) {
                                        return 181.0/156.0;
                                    } else {
                                        return 136.0/194.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 231327.0f ) {
                                        if ( cl->stats.size_rel <= 0.518878042698f ) {
                                            return 232.0/148.0;
                                        } else {
                                            return 241.0/78.0;
                                        }
                                    } else {
                                        return 206.0/28.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 181.0/210.0;
                                } else {
                                    return 199.0/116.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 252262.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                if ( cl->size() <= 9.5f ) {
                                    return 145.0/252.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 141.5f ) {
                                            if ( cl->stats.size_rel <= 0.782559275627f ) {
                                                return 268.0/174.0;
                                            } else {
                                                return 202.0/46.0;
                                            }
                                        } else {
                                            return 196.0/26.0;
                                        }
                                    } else {
                                        return 199.0/148.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 88.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 103601.0f ) {
                                        return 149.0/130.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                            return 216.0/54.0;
                                        } else {
                                            return 179.0/78.0;
                                        }
                                    }
                                } else {
                                    return 325.0/96.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 75.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                    return 254.0/90.0;
                                } else {
                                    return 218.0/16.0;
                                }
                            } else {
                                return 281.0/34.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 8.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.224989295006f ) {
                            return 151.0/204.0;
                        } else {
                            return 250.0/202.0;
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.84178674221f ) {
                            if ( cl->stats.size_rel <= 0.62585246563f ) {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    return 213.0/104.0;
                                } else {
                                    return 285.0/64.0;
                                }
                            } else {
                                return 182.0/138.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 83779.5f ) {
                                return 169.0/46.0;
                            } else {
                                return 192.0/34.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.931196331978f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.269762277603f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.24424508214f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.207237273455f ) {
                                    return 225.0/100.0;
                                } else {
                                    return 245.0/56.0;
                                }
                            } else {
                                return 267.0/174.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 97246.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.773034691811f ) {
                                            return 203.0/44.0;
                                        } else {
                                            return 173.0/54.0;
                                        }
                                    } else {
                                        return 222.0/18.0;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.763496279716f ) {
                                        return 305.0/72.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 135.5f ) {
                                            return 173.0/80.0;
                                        } else {
                                            return 196.0/70.0;
                                        }
                                    }
                                }
                            } else {
                                return 232.0/30.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 116.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.242448985577f ) {
                                if ( cl->stats.glue_rel_long <= 1.07951593399f ) {
                                    return 279.0/116.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.381575763226f ) {
                                        if ( cl->stats.glue <= 17.5f ) {
                                            return 189.0/44.0;
                                        } else {
                                            return 240.0/84.0;
                                        }
                                    } else {
                                        return 208.0/24.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 508694.5f ) {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 268.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                                return 313.0/98.0;
                                            } else {
                                                if ( cl->stats.dump_number <= 7.5f ) {
                                                    if ( cl->stats.size_rel <= 0.667815327644f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 33621.5f ) {
                                                            return 182.0/54.0;
                                                        } else {
                                                            return 198.0/42.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.37721037865f ) {
                                                            if ( cl->stats.num_overlap_literals <= 58.5f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 84.5f ) {
                                                                    return 221.0/24.0;
                                                                } else {
                                                                    return 189.0/50.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals_rel <= 0.768076181412f ) {
                                                                    return 195.0/28.0;
                                                                } else {
                                                                    return 406.0/18.0;
                                                                }
                                                            }
                                                        } else {
                                                            return 257.0/54.0;
                                                        }
                                                    }
                                                } else {
                                                    return 362.0/32.0;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 24782.5f ) {
                                                    return 378.0/34.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 63420.0f ) {
                                                        if ( cl->stats.glue_rel_long <= 1.1954741478f ) {
                                                            return 1;
                                                        } else {
                                                            return 192.0/8.0;
                                                        }
                                                    } else {
                                                        return 184.0/8.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.63726735115f ) {
                                                    if ( cl->size() <= 63.5f ) {
                                                        return 299.0/2.0;
                                                    } else {
                                                        if ( rdb0_last_touched_diff <= 40830.5f ) {
                                                            return 185.0/52.0;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 1.57056832314f ) {
                                                                return 236.0/12.0;
                                                            } else {
                                                                return 198.0/32.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 203.0/52.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 177.5f ) {
                                            if ( cl->size() <= 35.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.516600072384f ) {
                                                        return 243.0/24.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 89.5f ) {
                                                            return 287.0/4.0;
                                                        } else {
                                                            return 226.0/12.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                        if ( cl->stats.num_antecedents_rel <= 0.8991574049f ) {
                                                            if ( cl->stats.glue_rel_long <= 1.16355586052f ) {
                                                                return 251.0/20.0;
                                                            } else {
                                                                return 195.0/32.0;
                                                            }
                                                        } else {
                                                            return 239.0/94.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.dump_number <= 22.5f ) {
                                                            return 297.0/8.0;
                                                        } else {
                                                            return 216.0/28.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.dump_number <= 14.5f ) {
                                                    return 306.0/4.0;
                                                } else {
                                                    if ( cl->size() <= 78.5f ) {
                                                        if ( cl->size() <= 51.5f ) {
                                                            return 306.0/16.0;
                                                        } else {
                                                            return 193.0/24.0;
                                                        }
                                                    } else {
                                                        return 242.0/2.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 14.5f ) {
                                                return 263.0/30.0;
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 158548.0f ) {
                                                    if ( rdb0_last_touched_diff <= 144232.5f ) {
                                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                            return 1;
                                                        } else {
                                                            return 270.0/20.0;
                                                        }
                                                    } else {
                                                        return 199.0/18.0;
                                                    }
                                                } else {
                                                    if ( cl->size() <= 51.5f ) {
                                                        return 291.0/10.0;
                                                    } else {
                                                        return 1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 28.5f ) {
                                        return 186.0/80.0;
                                    } else {
                                        return 205.0/26.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                if ( cl->stats.glue <= 13.5f ) {
                                    return 210.0/108.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.672940433025f ) {
                                        return 210.0/68.0;
                                    } else {
                                        return 306.0/52.0;
                                    }
                                }
                            } else {
                                return 313.0/48.0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
            if ( cl->stats.size_rel <= 0.624184548855f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 3066097.5f ) {
                    if ( cl->stats.dump_number <= 12.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.87413215637f ) {
                            if ( cl->stats.glue_rel_long <= 0.564017236233f ) {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.429336518049f ) {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.234379932284f ) {
                                                if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                                    return 126.0/515.9;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.190292164683f ) {
                                                        return 83.0/477.9;
                                                    } else {
                                                        return 45.0/623.9;
                                                    }
                                                }
                                            } else {
                                                return 89.0/290.0;
                                            }
                                        } else {
                                            return 26.0/355.9;
                                        }
                                    } else {
                                        return 107.0/455.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.288979113102f ) {
                                        if ( cl->stats.dump_number <= 6.5f ) {
                                            return 66.0/521.9;
                                        } else {
                                            return 105.0/479.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 22577.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                return 47.0/331.9;
                                            } else {
                                                return 157.0/513.9;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                                    return 174.0/278.0;
                                                } else {
                                                    return 108.0/262.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                                    return 95.0/329.9;
                                                } else {
                                                    return 57.0/355.9;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                            return 118.0/511.9;
                                        } else {
                                            return 171.0/433.9;
                                        }
                                    } else {
                                        return 59.0/671.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.65395116806f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.166364639997f ) {
                                            return 77.0/260.0;
                                        } else {
                                            return 100.0/202.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                            return 150.0/407.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                                return 106.0/216.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.368345499039f ) {
                                                    return 121.0/192.0;
                                                } else {
                                                    return 206.0/206.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                return 186.0/150.0;
                            } else {
                                return 124.0/305.9;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0736477673054f ) {
                                        return 102.0/274.0;
                                    } else {
                                        return 130.0/571.9;
                                    }
                                } else {
                                    return 141.0/305.9;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 914415.0f ) {
                                    return 178.0/170.0;
                                } else {
                                    return 130.0/321.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 249783.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.208549693227f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1256606.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0740794837475f ) {
                                            if ( cl->size() <= 5.5f ) {
                                                return 158.0/226.0;
                                            } else {
                                                return 286.0/166.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.166323125362f ) {
                                                return 96.0/244.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.125586181879f ) {
                                                    return 130.0/212.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.539308547974f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0932501256466f ) {
                                                            return 140.0/124.0;
                                                        } else {
                                                            return 121.0/174.0;
                                                        }
                                                    } else {
                                                        return 301.0/254.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 29.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.402638077736f ) {
                                                return 88.0/260.0;
                                            } else {
                                                return 91.0/357.9;
                                            }
                                        } else {
                                            return 220.0/216.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.187714099884f ) {
                                        return 118.0/208.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                            if ( cl->stats.glue <= 10.5f ) {
                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                    return 221.0/224.0;
                                                } else {
                                                    if ( cl->size() <= 10.5f ) {
                                                        return 233.0/172.0;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.190592557192f ) {
                                                            return 203.0/110.0;
                                                        } else {
                                                            return 181.0/74.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 263.0/114.0;
                                            }
                                        } else {
                                            return 164.0/299.9;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 303070.0f ) {
                                    return 179.0/70.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.101288348436f ) {
                                        return 189.0/88.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 474510.5f ) {
                                            return 333.0/170.0;
                                        } else {
                                            return 162.0/188.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                        if ( rdb0_last_touched_diff <= 124421.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 42843664.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 134.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                                            return 83.0/383.9;
                                        } else {
                                            return 116.0/276.0;
                                        }
                                    } else {
                                        return 50.0/361.9;
                                    }
                                } else {
                                    return 57.0/595.9;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.42570590973f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0460746511817f ) {
                                        return 140.0/399.9;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 13097125.0f ) {
                                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                if ( cl->stats.dump_number <= 30.5f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 5275360.5f ) {
                                                        return 56.0/298.0;
                                                    } else {
                                                        return 41.0/329.9;
                                                    }
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 6953542.0f ) {
                                                        return 149.0/154.0;
                                                    } else {
                                                        return 99.0/250.0;
                                                    }
                                                }
                                            } else {
                                                return 113.0/559.9;
                                            }
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 150.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.065902531147f ) {
                                                    return 109.0/250.0;
                                                } else {
                                                    return 79.0/385.9;
                                                }
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 243.5f ) {
                                                    return 49.0/359.9;
                                                } else {
                                                    if ( cl->size() <= 6.5f ) {
                                                        return 21.0/373.9;
                                                    } else {
                                                        return 26.0/381.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 91.0/252.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.304651260376f ) {
                                if ( cl->size() <= 14.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 10410808.0f ) {
                                        return 163.0/264.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 241760.5f ) {
                                            return 81.0/315.9;
                                        } else {
                                            return 101.0/210.0;
                                        }
                                    }
                                } else {
                                    return 147.0/186.0;
                                }
                            } else {
                                return 157.0/180.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0137543790042f ) {
                            return 66.0/395.9;
                        } else {
                            if ( cl->stats.dump_number <= 24.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0953659117222f ) {
                                    if ( cl->stats.glue_rel_long <= 0.345621526241f ) {
                                        return 21.0/393.9;
                                    } else {
                                        return 17.0/621.9;
                                    }
                                } else {
                                    return 32.0/441.9;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                    return 124.0/541.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 11166787.0f ) {
                                        return 53.0/321.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                            if ( rdb0_last_touched_diff <= 1398.5f ) {
                                                return 5.0/429.9;
                                            } else {
                                                return 40.0/603.9;
                                            }
                                        } else {
                                            return 52.0/465.9;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.sum_uip1_used <= 10.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.714254021645f ) {
                            return 180.0/309.9;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 40425.5f ) {
                                return 201.0/323.9;
                            } else {
                                return 231.0/142.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 73.5f ) {
                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                return 192.0/403.9;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 69.5f ) {
                                    return 74.0/419.9;
                                } else {
                                    return 101.0/387.9;
                                }
                            }
                        } else {
                            return 60.0/659.9;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 54.5f ) {
                        if ( cl->stats.sum_uip1_used <= 28.5f ) {
                            if ( rdb0_last_touched_diff <= 84722.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 53592.5f ) {
                                    if ( cl->stats.size_rel <= 0.856527209282f ) {
                                        return 139.0/194.0;
                                    } else {
                                        return 200.0/62.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                            return 184.0/351.9;
                                        } else {
                                            return 160.0/204.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                            return 131.0/168.0;
                                        } else {
                                            return 218.0/152.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.380944907665f ) {
                                    if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                            if ( rdb0_last_touched_diff <= 169848.0f ) {
                                                return 156.0/76.0;
                                            } else {
                                                return 209.0/64.0;
                                            }
                                        } else {
                                            return 222.0/230.0;
                                        }
                                    } else {
                                        return 223.0/88.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.303456664085f ) {
                                        return 175.0/92.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.428091108799f ) {
                                            return 199.0/46.0;
                                        } else {
                                            return 162.0/80.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue <= 16.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.173749998212f ) {
                                    return 105.0/415.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.737644433975f ) {
                                        return 133.0/250.0;
                                    } else {
                                        return 91.0/260.0;
                                    }
                                }
                            } else {
                                return 165.0/248.0;
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 542332.5f ) {
                                if ( cl->stats.glue <= 14.5f ) {
                                    return 190.0/148.0;
                                } else {
                                    return 201.0/52.0;
                                }
                            } else {
                                return 185.0/278.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 379304.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 56665.0f ) {
                                    return 182.0/100.0;
                                } else {
                                    if ( cl->stats.size_rel <= 1.3397397995f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.671687304974f ) {
                                            return 183.0/84.0;
                                        } else {
                                            return 220.0/54.0;
                                        }
                                    } else {
                                        return 202.0/26.0;
                                    }
                                }
                            } else {
                                return 260.0/244.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->size() <= 8.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 3177846.0f ) {
                            if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                if ( rdb0_last_touched_diff <= 11521.5f ) {
                                    return 118.0/351.9;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 66.0/337.9;
                                    } else {
                                        return 106.0/317.9;
                                    }
                                }
                            } else {
                                return 74.0/509.9;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 87.5f ) {
                                return 92.0/385.9;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 5584.5f ) {
                                    return 25.0/515.9;
                                } else {
                                    return 36.0/353.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.0290910378098f ) {
                            return 61.0/347.9;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 29.5f ) {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.36047527194f ) {
                                        return 60.0/313.9;
                                    } else {
                                        return 41.0/643.9;
                                    }
                                } else {
                                    return 81.0/264.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 41663792.0f ) {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                            return 17.0/413.9;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 79.5f ) {
                                                return 51.0/585.9;
                                            } else {
                                                return 28.0/537.9;
                                            }
                                        }
                                    } else {
                                        return 56.0/377.9;
                                    }
                                } else {
                                    return 31.0/703.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 13.5f ) {
                        if ( cl->stats.sum_uip1_used <= 120.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 6907.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.00761173013598f ) {
                                    return 43.0/343.9;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2382.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 6727692.5f ) {
                                            if ( cl->size() <= 6.5f ) {
                                                if ( cl->stats.glue <= 4.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0922450497746f ) {
                                                        if ( cl->stats.dump_number <= 5.5f ) {
                                                            return 19.0/455.9;
                                                        } else {
                                                            return 11.0/405.9;
                                                        }
                                                    } else {
                                                        return 25.0/381.9;
                                                    }
                                                } else {
                                                    return 5.0/433.9;
                                                }
                                            } else {
                                                return 41.0/729.9;
                                            }
                                        } else {
                                            return 34.0/423.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 3.5f ) {
                                            return 57.0/651.9;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.494560331106f ) {
                                                if ( rdb0_last_touched_diff <= 2482.5f ) {
                                                    return 14.0/429.9;
                                                } else {
                                                    return 28.0/335.9;
                                                }
                                            } else {
                                                return 58.0/709.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 8194.5f ) {
                                    return 54.0/313.9;
                                } else {
                                    return 38.0/373.9;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.0821936577559f ) {
                                return 38.0/585.9;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0405185371637f ) {
                                    return 26.0/399.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0792458057404f ) {
                                        return 17.0/369.9;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.12293536216f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 240.0f ) {
                                                    return 5.0/467.9;
                                                } else {
                                                    return 2.0/639.9;
                                                }
                                            } else {
                                                return 8.0/463.9;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 2636.0f ) {
                                                return 15.0/377.9;
                                            } else {
                                                return 5.0/421.9;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 1263350.0f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 780.0f ) {
                                return 28.0/487.9;
                            } else {
                                return 23.0/655.9;
                            }
                        } else {
                            if ( cl->size() <= 6.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0802711769938f ) {
                                        return 26.0/459.9;
                                    } else {
                                        return 10.0/711.9;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0480116009712f ) {
                                        if ( cl->stats.size_rel <= 0.0915436223149f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.024737611413f ) {
                                                return 0.0/641.9;
                                            } else {
                                                return 14.0/611.9;
                                            }
                                        } else {
                                            return 16.0/525.9;
                                        }
                                    } else {
                                        if ( cl->size() <= 4.5f ) {
                                            return 0.0/889.9;
                                        } else {
                                            return 3.0/795.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    return 26.0/705.9;
                                } else {
                                    return 2.0/417.9;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 6096.0f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                        if ( cl->stats.sum_uip1_used <= 23.5f ) {
                            if ( cl->stats.size_rel <= 0.688121438026f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 2930.5f ) {
                                    return 81.0/681.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0634762942791f ) {
                                        return 55.0/333.9;
                                    } else {
                                        return 109.0/407.9;
                                    }
                                }
                            } else {
                                return 173.0/407.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.672619044781f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.142304837704f ) {
                                        return 65.0/529.9;
                                    } else {
                                        return 29.0/469.9;
                                    }
                                } else {
                                    return 89.0/595.9;
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 54.0/317.9;
                                    } else {
                                        return 30.0/417.9;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 41.5f ) {
                                        return 52.0/609.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.259994208813f ) {
                                            return 17.0/373.9;
                                        } else {
                                            return 11.0/383.9;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 2.5f ) {
                            if ( cl->size() <= 37.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                    return 57.0/435.9;
                                } else {
                                    return 23.0/483.9;
                                }
                            } else {
                                return 20.0/433.9;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 106.5f ) {
                                if ( cl->stats.size_rel <= 0.286335736513f ) {
                                    return 19.0/595.9;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 1655.0f ) {
                                        return 31.0/555.9;
                                    } else {
                                        return 47.0/465.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 13.5f ) {
                                    if ( rdb0_last_touched_diff <= 784.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 31.5f ) {
                                            return 10.0/533.9;
                                        } else {
                                            return 4.0/589.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 7.5f ) {
                                            return 13.0/527.9;
                                        } else {
                                            return 26.0/519.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 28.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 18.5f ) {
                                            return 18.0/709.9;
                                        } else {
                                            return 8.0/609.9;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 71.5f ) {
                                            return 0.0/743.9;
                                        } else {
                                            return 6.0/533.9;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 113.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                            if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                if ( cl->stats.size_rel <= 0.387827992439f ) {
                                    if ( cl->stats.dump_number <= 4.5f ) {
                                        return 105.0/345.9;
                                    } else {
                                        return 193.0/345.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 55.0f ) {
                                            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                                return 154.0/134.0;
                                            } else {
                                                return 177.0/278.0;
                                            }
                                        } else {
                                            return 104.0/194.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 18.5f ) {
                                            return 179.0/292.0;
                                        } else {
                                            return 148.0/333.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.125907123089f ) {
                                            return 58.0/389.9;
                                        } else {
                                            return 31.0/447.9;
                                        }
                                    } else {
                                        return 112.0/517.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.178598076105f ) {
                                        if ( cl->stats.sum_uip1_used <= 68.0f ) {
                                            return 150.0/248.0;
                                        } else {
                                            return 44.0/321.9;
                                        }
                                    } else {
                                        return 103.0/547.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 42.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.417803823948f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 64.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 55.5f ) {
                                            if ( rdb0_last_touched_diff <= 10703.5f ) {
                                                return 93.0/363.9;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.204861104488f ) {
                                                    return 46.0/389.9;
                                                } else {
                                                    return 69.0/341.9;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 10629.0f ) {
                                                return 15.0/529.9;
                                            } else {
                                                return 61.0/477.9;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 12141.0f ) {
                                            return 53.0/405.9;
                                        } else {
                                            return 77.0/282.0;
                                        }
                                    }
                                } else {
                                    return 71.0/317.9;
                                }
                            } else {
                                return 15.0/401.9;
                            }
                        }
                    } else {
                        return 173.0/224.0;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_2(
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
    if ( cl->stats.sum_delta_confl_uip1_used <= 4217.5f ) {
        if ( cl->size() <= 10.5f ) {
            if ( cl->stats.num_antecedents_rel <= 0.243637531996f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 75227.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                return 136.0/541.9;
                            } else {
                                return 34.0/389.9;
                            }
                        } else {
                            return 112.0/280.0;
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.0750610381365f ) {
                            return 169.0/230.0;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                if ( cl->stats.glue <= 4.5f ) {
                                    return 83.0/246.0;
                                } else {
                                    return 92.0/282.0;
                                }
                            } else {
                                return 120.0/150.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.514562547207f ) {
                        return 267.0/299.9;
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 147639.5f ) {
                            return 144.0/156.0;
                        } else {
                            return 177.0/90.0;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 117601.0f ) {
                    if ( cl->stats.glue_rel_long <= 0.757414638996f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 24984.0f ) {
                            return 171.0/403.9;
                        } else {
                            if ( cl->stats.glue <= 5.5f ) {
                                return 123.0/230.0;
                            } else {
                                return 142.0/132.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 91.5f ) {
                            return 187.0/236.0;
                        } else {
                            return 220.0/94.0;
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                        return 175.0/94.0;
                    } else {
                        return 311.0/104.0;
                    }
                }
            }
        } else {
            if ( cl->stats.glue_rel_long <= 0.85346186161f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.40985673666f ) {
                        if ( cl->stats.dump_number <= 11.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                    if ( cl->stats.size_rel <= 0.599384307861f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 28.5f ) {
                                            return 190.0/298.0;
                                        } else {
                                            return 208.0/184.0;
                                        }
                                    } else {
                                        return 330.0/208.0;
                                    }
                                } else {
                                    return 127.0/210.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                        return 133.0/148.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.67708349228f ) {
                                            return 171.0/152.0;
                                        } else {
                                            return 201.0/94.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 80488.5f ) {
                                        return 257.0/92.0;
                                    } else {
                                        return 207.0/104.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 298363.0f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 319.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                        return 320.0/150.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                            return 199.0/28.0;
                                        } else {
                                            return 206.0/62.0;
                                        }
                                    }
                                } else {
                                    return 163.0/108.0;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                    return 249.0/76.0;
                                } else {
                                    return 228.0/24.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 12924.0f ) {
                                    return 152.0/102.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                        return 271.0/64.0;
                                    } else {
                                        return 224.0/104.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 174.0/76.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 357.0f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 9.20486068726f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                return 261.0/18.0;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.795862674713f ) {
                                                    return 278.0/26.0;
                                                } else {
                                                    return 178.0/64.0;
                                                }
                                            }
                                        } else {
                                            return 205.0/64.0;
                                        }
                                    } else {
                                        return 247.0/92.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 728.5f ) {
                                return 208.0/106.0;
                            } else {
                                return 138.0/140.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.595738172531f ) {
                        return 99.0/264.0;
                    } else {
                        return 169.0/260.0;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 1.0792683363f ) {
                    if ( cl->stats.num_overlap_literals <= 17.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    return 180.0/124.0;
                                } else {
                                    return 139.0/152.0;
                                }
                            } else {
                                return 210.0/70.0;
                            }
                        } else {
                            return 192.0/54.0;
                        }
                    } else {
                        if ( cl->stats.dump_number <= 3.5f ) {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 6.43381118774f ) {
                                    return 242.0/112.0;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.93094432354f ) {
                                        return 282.0/64.0;
                                    } else {
                                        return 284.0/32.0;
                                    }
                                }
                            } else {
                                return 297.0/192.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 8.91232299805f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( rdb0_last_touched_diff <= 94427.5f ) {
                                            return 327.0/64.0;
                                        } else {
                                            return 314.0/36.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 229263.0f ) {
                                            if ( rdb0_last_touched_diff <= 131885.0f ) {
                                                return 235.0/50.0;
                                            } else {
                                                return 189.0/84.0;
                                            }
                                        } else {
                                            return 233.0/30.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.size_rel <= 1.13761639595f ) {
                                            if ( cl->stats.glue_rel_long <= 0.963285684586f ) {
                                                return 238.0/34.0;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.0371325016f ) {
                                                    return 240.0/16.0;
                                                } else {
                                                    return 230.0/6.0;
                                                }
                                            }
                                        } else {
                                            return 384.0/6.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 141.5f ) {
                                            return 218.0/24.0;
                                        } else {
                                            return 207.0/50.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.60843038559f ) {
                                    if ( cl->size() <= 27.5f ) {
                                        return 271.0/82.0;
                                    } else {
                                        return 186.0/88.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 49.0f ) {
                                        return 180.0/48.0;
                                    } else {
                                        return 297.0/48.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue <= 10.5f ) {
                        if ( cl->stats.size_rel <= 0.886847913265f ) {
                            if ( cl->stats.num_overlap_literals <= 41.5f ) {
                                return 172.0/92.0;
                            } else {
                                return 189.0/60.0;
                            }
                        } else {
                            return 264.0/42.0;
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.46423971653f ) {
                            if ( cl->stats.size_rel <= 1.46834826469f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 25697.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 37.0f ) {
                                        return 162.0/104.0;
                                    } else {
                                        return 174.0/76.0;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 8.7109375f ) {
                                            return 291.0/64.0;
                                        } else {
                                            return 374.0/24.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue <= 24.5f ) {
                                            return 267.0/94.0;
                                        } else {
                                            return 183.0/32.0;
                                        }
                                    }
                                }
                            } else {
                                return 203.0/8.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 320552.5f ) {
                                if ( cl->stats.glue_rel_long <= 1.18727779388f ) {
                                    if ( cl->stats.size_rel <= 1.14985275269f ) {
                                        if ( cl->size() <= 41.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 1.24529957771f ) {
                                                return 379.0/22.0;
                                            } else {
                                                return 264.0/4.0;
                                            }
                                        } else {
                                            return 275.0/52.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 479.5f ) {
                                            if ( rdb0_last_touched_diff <= 61593.5f ) {
                                                return 212.0/6.0;
                                            } else {
                                                return 1;
                                            }
                                        } else {
                                            return 279.0/16.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 2.12783098221f ) {
                                        if ( cl->stats.dump_number <= 5.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 1.43694496155f ) {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    if ( cl->stats.size_rel <= 1.22318840027f ) {
                                                        return 273.0/26.0;
                                                    } else {
                                                        return 310.0/4.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 229.5f ) {
                                                        return 272.0/26.0;
                                                    } else {
                                                        return 268.0/68.0;
                                                    }
                                                }
                                            } else {
                                                return 210.0/54.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_long <= 1.2423106432f ) {
                                                return 307.0/56.0;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 113.5f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.944801390171f ) {
                                                        if ( cl->stats.dump_number <= 18.5f ) {
                                                            if ( cl->stats.num_antecedents_rel <= 0.538821458817f ) {
                                                                return 235.0/30.0;
                                                            } else {
                                                                return 234.0/6.0;
                                                            }
                                                        } else {
                                                            return 1;
                                                        }
                                                    } else {
                                                        return 197.0/50.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 143258.5f ) {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.20297300816f ) {
                                                            return 286.0/6.0;
                                                        } else {
                                                            return 338.0/38.0;
                                                        }
                                                    } else {
                                                        return 371.0/8.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 37605.0f ) {
                                            return 396.0/36.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 1.92816781998f ) {
                                                return 207.0/16.0;
                                            } else {
                                                if ( cl->size() <= 35.5f ) {
                                                    return 179.0/8.0;
                                                } else {
                                                    return 375.0/2.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                    return 316.0/28.0;
                                } else {
                                    if ( cl->stats.glue <= 17.5f ) {
                                        return 211.0/94.0;
                                    } else {
                                        return 237.0/26.0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 21.5f ) {
            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 58890.5f ) {
                    if ( cl->stats.glue <= 9.5f ) {
                        if ( rdb0_last_touched_diff <= 29779.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.405381917953f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                        if ( cl->stats.size_rel <= 0.169339433312f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.044541798532f ) {
                                                return 109.0/307.9;
                                            } else {
                                                return 86.0/435.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                if ( cl->stats.glue <= 6.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.534332633018f ) {
                                                        if ( rdb0_last_touched_diff <= 18772.0f ) {
                                                            return 95.0/220.0;
                                                        } else {
                                                            return 79.0/240.0;
                                                        }
                                                    } else {
                                                        return 65.0/264.0;
                                                    }
                                                } else {
                                                    return 63.0/347.9;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 104.0/266.0;
                                                } else {
                                                    return 109.0/192.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 211.0/373.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                        if ( cl->stats.size_rel <= 0.391908466816f ) {
                                            return 189.0/455.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 18284.0f ) {
                                                return 161.0/268.0;
                                            } else {
                                                return 236.0/242.0;
                                            }
                                        }
                                    } else {
                                        return 99.0/323.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.513865709305f ) {
                                    return 63.0/477.9;
                                } else {
                                    return 81.0/307.9;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.64883518219f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 60520.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                        return 196.0/373.9;
                                    } else {
                                        return 134.0/501.9;
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 189.0/423.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            return 170.0/166.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 18.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                                    return 145.0/232.0;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 322683.5f ) {
                                                        return 99.0/301.9;
                                                    } else {
                                                        return 112.0/216.0;
                                                    }
                                                }
                                            } else {
                                                return 260.0/307.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 17.5f ) {
                                    return 191.0/174.0;
                                } else {
                                    return 198.0/242.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.531742095947f ) {
                            if ( cl->stats.size_rel <= 0.979103207588f ) {
                                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                    return 193.0/294.0;
                                } else {
                                    return 162.0/505.9;
                                }
                            } else {
                                return 143.0/180.0;
                            }
                        } else {
                            if ( cl->size() <= 129.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 263.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.843654036522f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 143.0/288.0;
                                        } else {
                                            return 235.0/270.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6637.5f ) {
                                            return 131.0/160.0;
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                return 255.0/220.0;
                                            } else {
                                                return 312.0/170.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 208.0/124.0;
                                }
                            } else {
                                return 195.0/64.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.784188330173f ) {
                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                            if ( cl->size() <= 11.5f ) {
                                return 119.0/220.0;
                            } else {
                                return 210.0/216.0;
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 259682.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.509634017944f ) {
                                    if ( cl->stats.dump_number <= 21.5f ) {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.size_rel <= 0.193257629871f ) {
                                                return 94.0/212.0;
                                            } else {
                                                return 136.0/204.0;
                                            }
                                        } else {
                                            return 136.0/138.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0518532991409f ) {
                                            return 199.0/102.0;
                                        } else {
                                            return 225.0/220.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 133.0/136.0;
                                        } else {
                                            return 122.0/172.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.187558144331f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 128042.0f ) {
                                                return 184.0/94.0;
                                            } else {
                                                return 177.0/58.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
                                                return 173.0/172.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 91.0f ) {
                                                    return 270.0/178.0;
                                                } else {
                                                    return 220.0/98.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 578003.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 29.5f ) {
                                        return 273.0/164.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.613175153732f ) {
                                            return 230.0/36.0;
                                        } else {
                                            return 162.0/50.0;
                                        }
                                    }
                                } else {
                                    return 163.0/142.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 16.5f ) {
                            if ( cl->stats.glue <= 7.5f ) {
                                return 209.0/202.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 137454.0f ) {
                                    return 232.0/90.0;
                                } else {
                                    return 200.0/156.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 191.0/76.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 1.04534590244f ) {
                                        return 260.0/68.0;
                                    } else {
                                        return 334.0/28.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 73.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 167175.0f ) {
                                        return 268.0/168.0;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 155442.5f ) {
                                            return 164.0/84.0;
                                        } else {
                                            return 178.0/60.0;
                                        }
                                    }
                                } else {
                                    return 221.0/56.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 136597.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 28101.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 4.71241950989f ) {
                                if ( cl->stats.size_rel <= 0.388168513775f ) {
                                    if ( cl->stats.glue_rel_long <= 0.558677315712f ) {
                                        return 74.0/555.9;
                                    } else {
                                        return 72.0/319.9;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.274585098028f ) {
                                        return 95.0/262.0;
                                    } else {
                                        return 106.0/232.0;
                                    }
                                }
                            } else {
                                return 126.0/174.0;
                            }
                        } else {
                            return 180.0/208.0;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 3282.0f ) {
                            return 112.0/234.0;
                        } else {
                            return 206.0/212.0;
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 0.749919176102f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.dump_number <= 12.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.395258456469f ) {
                                                return 48.0/411.9;
                                            } else {
                                                return 75.0/433.9;
                                            }
                                        } else {
                                            return 38.0/405.9;
                                        }
                                    } else {
                                        return 59.0/347.9;
                                    }
                                } else {
                                    return 124.0/483.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 3094.5f ) {
                                    return 97.0/278.0;
                                } else {
                                    return 181.0/315.9;
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.size_rel <= 0.138035297394f ) {
                                    return 53.0/321.9;
                                } else {
                                    if ( cl->size() <= 7.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 153569.0f ) {
                                            return 23.0/383.9;
                                        } else {
                                            return 18.0/429.9;
                                        }
                                    } else {
                                        return 38.0/405.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    return 103.0/435.9;
                                } else {
                                    return 43.0/343.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            return 120.0/527.9;
                        } else {
                            if ( rdb0_last_touched_diff <= 4132.5f ) {
                                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                    return 89.0/216.0;
                                } else {
                                    return 108.0/427.9;
                                }
                            } else {
                                return 212.0/367.9;
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 43033.0f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( rdb0_last_touched_diff <= 6090.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 22991220.0f ) {
                            if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0308916736394f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 3992.0f ) {
                                        return 37.0/355.9;
                                    } else {
                                        return 67.0/319.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 724.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 69.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.536411881447f ) {
                                                    return 56.0/333.9;
                                                } else {
                                                    return 66.0/284.0;
                                                }
                                            } else {
                                                return 29.0/461.9;
                                            }
                                        } else {
                                            return 16.0/409.9;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 29.5f ) {
                                            if ( cl->size() <= 11.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.159387171268f ) {
                                                    if ( cl->stats.sum_uip1_used <= 84.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0307336878031f ) {
                                                            return 32.0/413.9;
                                                        } else {
                                                            if ( cl->stats.glue_rel_queue <= 0.544070243835f ) {
                                                                if ( cl->stats.dump_number <= 9.5f ) {
                                                                    return 14.0/471.9;
                                                                } else {
                                                                    return 20.0/437.9;
                                                                }
                                                            } else {
                                                                return 23.0/361.9;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.115430429578f ) {
                                                            return 11.0/373.9;
                                                        } else {
                                                            return 6.0/417.9;
                                                        }
                                                    }
                                                } else {
                                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                        return 20.0/355.9;
                                                    } else {
                                                        return 29.0/375.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                                    return 91.0/563.9;
                                                } else {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 3363547.5f ) {
                                                        return 56.0/487.9;
                                                    } else {
                                                        return 19.0/485.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 2923.5f ) {
                                                return 86.0/363.9;
                                            } else {
                                                return 71.0/523.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.213151931763f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.508509337902f ) {
                                            if ( cl->stats.used_for_uip_creation <= 20.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0546016618609f ) {
                                                    return 36.0/341.9;
                                                } else {
                                                    return 48.0/729.9;
                                                }
                                            } else {
                                                return 28.0/721.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                                return 9.0/579.9;
                                            } else {
                                                return 16.0/459.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 152.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0999883562326f ) {
                                                if ( cl->stats.size_rel <= 0.0924153625965f ) {
                                                    return 15.0/603.9;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 1448.5f ) {
                                                        return 20.0/773.9;
                                                    } else {
                                                        return 35.0/399.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.201077982783f ) {
                                                    return 12.0/519.9;
                                                } else {
                                                    return 2.0/575.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 456.0f ) {
                                                    return 1.0/663.9;
                                                } else {
                                                    return 7.0/719.9;
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 10142815.0f ) {
                                                    return 5.0/459.9;
                                                } else {
                                                    return 15.0/387.9;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1854.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                            if ( cl->size() <= 13.5f ) {
                                                return 31.0/453.9;
                                            } else {
                                                return 18.0/509.9;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1057230.0f ) {
                                                return 17.0/379.9;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.47785153985f ) {
                                                    return 13.0/493.9;
                                                } else {
                                                    return 1.0/595.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 25.5f ) {
                                            return 81.0/575.9;
                                        } else {
                                            return 34.0/443.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0735616981983f ) {
                                            return 39.0/731.9;
                                        } else {
                                            return 45.0/475.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.5024112463f ) {
                                            return 26.0/557.9;
                                        } else {
                                            return 4.0/435.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0566235259175f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1926.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 355.5f ) {
                                                    return 9.0/439.9;
                                                } else {
                                                    return 1.0/387.9;
                                                }
                                            } else {
                                                return 13.0/409.9;
                                            }
                                        } else {
                                            return 24.0/399.9;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 4156.5f ) {
                                            if ( cl->stats.dump_number <= 91.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 6712.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 49.5f ) {
                                                        return 10.0/489.9;
                                                    } else {
                                                        if ( cl->stats.glue <= 15.5f ) {
                                                            if ( cl->stats.glue_rel_queue <= 0.211065113544f ) {
                                                                return 7.0/381.9;
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals_rel <= 0.0848728269339f ) {
                                                                    if ( cl->stats.num_antecedents_rel <= 0.161055237055f ) {
                                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0452813357115f ) {
                                                                            return 0.0/855.9;
                                                                        } else {
                                                                            return 3.0/405.9;
                                                                        }
                                                                    } else {
                                                                        return 6.0/435.9;
                                                                    }
                                                                } else {
                                                                    return 0.0/1029.8;
                                                                }
                                                            }
                                                        } else {
                                                            return 7.0/425.9;
                                                        }
                                                    }
                                                } else {
                                                    return 15.0/399.9;
                                                }
                                            } else {
                                                return 11.0/501.9;
                                            }
                                        } else {
                                            return 15.0/441.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    return 51.0/567.9;
                                } else {
                                    return 31.0/431.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                            if ( rdb0_last_touched_diff <= 11529.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.249934345484f ) {
                                    return 67.0/465.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 42013484.0f ) {
                                        if ( cl->stats.size_rel <= 0.398356497288f ) {
                                            if ( cl->stats.size_rel <= 0.205415576696f ) {
                                                return 55.0/637.9;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.145436286926f ) {
                                                    return 44.0/441.9;
                                                } else {
                                                    return 50.0/286.0;
                                                }
                                            }
                                        } else {
                                            return 14.0/389.9;
                                        }
                                    } else {
                                        return 22.0/747.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.176138669252f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0891623049974f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0676598995924f ) {
                                            return 92.0/631.9;
                                        } else {
                                            return 62.0/292.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.301188528538f ) {
                                            return 45.0/353.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0557757243514f ) {
                                                return 30.0/495.9;
                                            } else {
                                                return 38.0/367.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 11554470.0f ) {
                                        return 67.0/321.9;
                                    } else {
                                        return 51.0/325.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.sum_uip1_used <= 190.0f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0379619896412f ) {
                                        return 111.0/236.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.170845866203f ) {
                                            return 141.0/369.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                                                return 64.0/469.9;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 78.5f ) {
                                                    return 83.0/290.0;
                                                } else {
                                                    return 68.0/361.9;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 43.0/475.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 8077.5f ) {
                                    return 39.0/553.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.0934788361192f ) {
                                        return 61.0/315.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 48.5f ) {
                                            return 72.0/405.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.17814874649f ) {
                                                return 44.0/323.9;
                                            } else {
                                                return 40.0/625.9;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.0342856869102f ) {
                            return 138.0/481.9;
                        } else {
                            if ( cl->stats.dump_number <= 22.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.28091531992f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.628437161446f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0362900793552f ) {
                                                return 20.0/343.9;
                                            } else {
                                                return 39.0/421.9;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 3158876.5f ) {
                                                if ( cl->stats.glue <= 5.5f ) {
                                                    return 73.0/349.9;
                                                } else {
                                                    return 62.0/553.9;
                                                }
                                            } else {
                                                return 24.0/427.9;
                                            }
                                        }
                                    } else {
                                        return 106.0/541.9;
                                    }
                                } else {
                                    if ( cl->size() <= 27.5f ) {
                                        return 69.0/367.9;
                                    } else {
                                        return 74.0/309.9;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 30.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 59.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.160662353039f ) {
                                            return 113.0/198.0;
                                        } else {
                                            return 106.0/333.9;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 46138048.0f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0603229701519f ) {
                                                return 66.0/296.0;
                                            } else {
                                                return 56.0/479.9;
                                            }
                                        } else {
                                            return 37.0/481.9;
                                        }
                                    }
                                } else {
                                    return 117.0/214.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 11.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.655639410019f ) {
                                if ( cl->stats.dump_number <= 21.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0601226389408f ) {
                                        return 34.0/353.9;
                                    } else {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            return 36.0/521.9;
                                        } else {
                                            return 13.0/373.9;
                                        }
                                    }
                                } else {
                                    return 45.0/311.9;
                                }
                            } else {
                                return 91.0/571.9;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.732643485069f ) {
                                if ( rdb0_last_touched_diff <= 7012.5f ) {
                                    return 15.0/589.9;
                                } else {
                                    return 53.0/727.9;
                                }
                            } else {
                                return 33.0/367.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 96.5f ) {
                    if ( cl->stats.dump_number <= 29.5f ) {
                        if ( cl->stats.glue <= 9.5f ) {
                            if ( rdb0_last_touched_diff <= 63150.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                    return 57.0/387.9;
                                } else {
                                    return 88.0/319.9;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                    if ( cl->stats.size_rel <= 0.216722041368f ) {
                                        return 89.0/319.9;
                                    } else {
                                        return 83.0/363.9;
                                    }
                                } else {
                                    return 185.0/387.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 77304.0f ) {
                                return 151.0/399.9;
                            } else {
                                return 182.0/230.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.21537438035f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0292571783066f ) {
                                    return 159.0/202.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 140416.5f ) {
                                        return 166.0/425.9;
                                    } else {
                                        return 86.0/272.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                    return 110.0/184.0;
                                } else {
                                    return 157.0/124.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.705431342125f ) {
                                return 215.0/313.9;
                            } else {
                                return 255.0/172.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 257956.5f ) {
                        if ( cl->stats.dump_number <= 65.5f ) {
                            if ( rdb0_last_touched_diff <= 84991.5f ) {
                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                    if ( cl->stats.dump_number <= 27.5f ) {
                                        return 29.0/353.9;
                                    } else {
                                        return 40.0/329.9;
                                    }
                                } else {
                                    return 79.0/429.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                    return 108.0/369.9;
                                } else {
                                    return 69.0/331.9;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 220.5f ) {
                                return 81.0/222.0;
                            } else {
                                return 74.0/337.9;
                            }
                        }
                    } else {
                        return 177.0/303.9;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_3(
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
    if ( cl->stats.sum_uip1_used <= 7.5f ) {
        if ( cl->stats.num_total_lits_antecedents <= 38.5f ) {
            if ( cl->stats.dump_number <= 7.5f ) {
                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( cl->stats.glue_rel_queue <= 0.645415306091f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 15251.0f ) {
                                    return 71.0/286.0;
                                } else {
                                    return 47.0/327.9;
                                }
                            } else {
                                return 78.0/629.9;
                            }
                        } else {
                            return 114.0/493.9;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                            return 181.0/501.9;
                        } else {
                            return 127.0/170.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 2.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 3.16319441795f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                if ( rdb0_last_touched_diff <= 54969.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.0777760595083f ) {
                                        return 141.0/222.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.127178117633f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 27.5f ) {
                                                return 189.0/365.9;
                                            } else {
                                                return 84.0/278.0;
                                            }
                                        } else {
                                            return 78.0/260.0;
                                        }
                                    }
                                } else {
                                    return 173.0/212.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.761493265629f ) {
                                    return 213.0/242.0;
                                } else {
                                    return 172.0/106.0;
                                }
                            }
                        } else {
                            return 167.0/84.0;
                        }
                    } else {
                        if ( cl->stats.glue <= 6.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0950232967734f ) {
                                return 94.0/439.9;
                            } else {
                                if ( cl->stats.glue <= 4.5f ) {
                                    return 106.0/363.9;
                                } else {
                                    return 145.0/311.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                return 133.0/323.9;
                            } else {
                                return 163.0/178.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->size() <= 9.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.0857688263059f ) {
                        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 75.0/280.0;
                            } else {
                                return 113.0/226.0;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.170332938433f ) {
                                if ( cl->stats.glue_rel_queue <= 0.386797428131f ) {
                                    return 251.0/202.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        return 221.0/313.9;
                                    } else {
                                        return 176.0/156.0;
                                    }
                                }
                            } else {
                                return 171.0/68.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 15.5f ) {
                            return 214.0/274.0;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 170801.0f ) {
                                return 130.0/126.0;
                            } else {
                                return 166.0/92.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                        if ( cl->stats.num_overlap_literals <= 4.5f ) {
                            return 243.0/162.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 553.5f ) {
                                if ( cl->stats.dump_number <= 25.5f ) {
                                    if ( cl->stats.size_rel <= 0.65851688385f ) {
                                        return 323.0/176.0;
                                    } else {
                                        return 273.0/42.0;
                                    }
                                } else {
                                    return 326.0/40.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 93937.0f ) {
                                    return 149.0/110.0;
                                } else {
                                    return 254.0/106.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.180819422007f ) {
                                return 173.0/140.0;
                            } else {
                                return 131.0/178.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                return 214.0/196.0;
                            } else {
                                return 194.0/72.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->size() <= 11.5f ) {
                if ( cl->stats.glue <= 8.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 19944.0f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 91.5f ) {
                            return 156.0/178.0;
                        } else {
                            if ( cl->stats.size_rel <= 0.229741245508f ) {
                                return 62.0/311.9;
                            } else {
                                return 95.0/224.0;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.14323925972f ) {
                            return 167.0/256.0;
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.807382822037f ) {
                                    return 203.0/292.0;
                                } else {
                                    return 156.0/110.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 7.5f ) {
                                    return 168.0/76.0;
                                } else {
                                    return 179.0/148.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.926706671715f ) {
                        return 210.0/158.0;
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.03155970573f ) {
                            return 205.0/42.0;
                        } else {
                            return 177.0/66.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 31798.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 692.5f ) {
                        if ( cl->stats.num_overlap_literals <= 32.5f ) {
                            if ( cl->stats.size_rel <= 0.771914362907f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.206299453974f ) {
                                    return 178.0/166.0;
                                } else {
                                    return 115.0/176.0;
                                }
                            } else {
                                if ( cl->size() <= 26.5f ) {
                                    return 201.0/34.0;
                                } else {
                                    return 268.0/190.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.851685166359f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 4.03624153137f ) {
                                    return 292.0/208.0;
                                } else {
                                    return 300.0/132.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.09973824024f ) {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        if ( rdb0_last_touched_diff <= 20627.0f ) {
                                            return 358.0/82.0;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 32338.0f ) {
                                                return 312.0/28.0;
                                            } else {
                                                return 187.0/32.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16884.5f ) {
                                            return 150.0/84.0;
                                        } else {
                                            return 211.0/86.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 1.10457825661f ) {
                                        if ( cl->stats.glue_rel_long <= 1.21830558777f ) {
                                            return 313.0/76.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.724457859993f ) {
                                                return 196.0/40.0;
                                            } else {
                                                return 287.0/20.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 305.0/4.0;
                                        } else {
                                            if ( cl->size() <= 79.0f ) {
                                                return 310.0/48.0;
                                            } else {
                                                return 382.0/18.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 88.5f ) {
                            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.425361573696f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.197410300374f ) {
                                        return 128.0/162.0;
                                    } else {
                                        return 88.0/242.0;
                                    }
                                } else {
                                    if ( cl->size() <= 18.5f ) {
                                        return 191.0/236.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.19579076767f ) {
                                            return 254.0/134.0;
                                        } else {
                                            return 175.0/146.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 4.5f ) {
                                    return 100.0/353.9;
                                } else {
                                    return 115.0/168.0;
                                }
                            }
                        } else {
                            if ( cl->size() <= 70.5f ) {
                                if ( rdb0_last_touched_diff <= 15734.5f ) {
                                    return 160.0/208.0;
                                } else {
                                    return 235.0/170.0;
                                }
                            } else {
                                return 234.0/116.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.46722221375f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 241.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.756876587868f ) {
                                    return 134.0/172.0;
                                } else {
                                    return 183.0/116.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.635345697403f ) {
                                    if ( cl->stats.size_rel <= 0.888972222805f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.252922415733f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.134094789624f ) {
                                                return 213.0/108.0;
                                            } else {
                                                return 234.0/68.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 8.5f ) {
                                                return 179.0/48.0;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.512175917625f ) {
                                                    return 110.0/162.0;
                                                } else {
                                                    return 146.0/124.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 252.0/66.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 94017.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.349008828402f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 28.5f ) {
                                                if ( cl->stats.size_rel <= 0.817285656929f ) {
                                                    return 218.0/32.0;
                                                } else {
                                                    return 191.0/62.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                                    return 142.0/126.0;
                                                } else {
                                                    return 190.0/106.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 8.5f ) {
                                                if ( cl->stats.size_rel <= 0.841133832932f ) {
                                                    return 181.0/56.0;
                                                } else {
                                                    return 371.0/64.0;
                                                }
                                            } else {
                                                return 187.0/90.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 567011.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.4911236763f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                                        return 223.0/20.0;
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.295321583748f ) {
                                                            return 340.0/96.0;
                                                        } else {
                                                            if ( cl->stats.glue_rel_long <= 1.06303215027f ) {
                                                                if ( rdb0_last_touched_diff <= 215524.5f ) {
                                                                    return 225.0/34.0;
                                                                } else {
                                                                    return 210.0/8.0;
                                                                }
                                                            } else {
                                                                return 250.0/56.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                            return 353.0/68.0;
                                                        } else {
                                                            if ( cl->size() <= 23.5f ) {
                                                                return 248.0/140.0;
                                                            } else {
                                                                return 264.0/78.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_antecedents_rel <= 0.798304319382f ) {
                                                            return 187.0/40.0;
                                                        } else {
                                                            return 230.0/10.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 243.0/96.0;
                                            }
                                        } else {
                                            return 260.0/106.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.762472629547f ) {
                                return 301.0/104.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 71497.5f ) {
                                    return 288.0/72.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.773491203785f ) {
                                        return 190.0/32.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 398.5f ) {
                                            return 214.0/26.0;
                                        } else {
                                            return 305.0/12.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 20.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 327.0/124.0;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.glue <= 10.5f ) {
                                            return 288.0/58.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.23939990997f ) {
                                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                    return 303.0/46.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 120394.5f ) {
                                                        if ( rdb0_last_touched_diff <= 79024.5f ) {
                                                            return 217.0/22.0;
                                                        } else {
                                                            return 192.0/36.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.09717583656f ) {
                                                            return 395.0/32.0;
                                                        } else {
                                                            return 228.0/2.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.43132996559f ) {
                                                    return 345.0/4.0;
                                                } else {
                                                    return 184.0/22.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 267857.5f ) {
                                            if ( cl->stats.size_rel <= 0.629176855087f ) {
                                                return 207.0/68.0;
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                    return 275.0/74.0;
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.20138573647f ) {
                                                        return 298.0/28.0;
                                                    } else {
                                                        return 211.0/6.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 358.0/112.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 54662.5f ) {
                                        return 180.0/104.0;
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.983995556831f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.764332413673f ) {
                                                return 236.0/116.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.684649348259f ) {
                                                    return 248.0/62.0;
                                                } else {
                                                    return 171.0/56.0;
                                                }
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 174033.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.772921204567f ) {
                                                    return 183.0/22.0;
                                                } else {
                                                    return 196.0/50.0;
                                                }
                                            } else {
                                                return 272.0/20.0;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.957301735878f ) {
                                return 344.0/96.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.701219081879f ) {
                                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                        return 354.0/12.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 215.5f ) {
                                            return 304.0/92.0;
                                        } else {
                                            return 362.0/32.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.880950450897f ) {
                                        return 1;
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 73.2877197266f ) {
                                                if ( rdb0_last_touched_diff <= 127056.5f ) {
                                                    return 1;
                                                } else {
                                                    return 207.0/2.0;
                                                }
                                            } else {
                                                if ( cl->stats.size_rel <= 1.28248214722f ) {
                                                    return 202.0/18.0;
                                                } else {
                                                    return 260.0/2.0;
                                                }
                                            }
                                        } else {
                                            if ( cl->size() <= 101.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 223.0f ) {
                                                    return 236.0/6.0;
                                                } else {
                                                    return 363.0/22.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.15700650215f ) {
                                                    return 183.0/40.0;
                                                } else {
                                                    return 348.0/30.0;
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
    } else {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( cl->stats.sum_delta_confl_uip1_used <= 10547670.0f ) {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( rdb0_last_touched_diff <= 7666.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.781801462173f ) {
                                if ( cl->size() <= 8.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 25508.5f ) {
                                            return 86.0/763.9;
                                        } else {
                                            return 69.0/307.9;
                                        }
                                    } else {
                                        return 34.0/363.9;
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        return 103.0/545.9;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1830278.5f ) {
                                            return 130.0/284.0;
                                        } else {
                                            return 116.0/507.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 11916.0f ) {
                                    return 78.0/266.0;
                                } else {
                                    return 168.0/274.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.312256246805f ) {
                                    return 32.0/407.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.76038146019f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 25499.0f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 10570.0f ) {
                                                if ( cl->size() <= 8.5f ) {
                                                    return 38.0/383.9;
                                                } else {
                                                    return 66.0/284.0;
                                                }
                                            } else {
                                                return 35.0/357.9;
                                            }
                                        } else {
                                            return 73.0/290.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.978376448154f ) {
                                            return 99.0/262.0;
                                        } else {
                                            return 64.0/301.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 41523.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0130061144009f ) {
                                        return 68.0/525.9;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2354.5f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 56.0/721.9;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.153355181217f ) {
                                                        return 11.0/787.9;
                                                    } else {
                                                        return 29.0/543.9;
                                                    }
                                                } else {
                                                    return 37.0/397.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.102642327547f ) {
                                                return 76.0/473.9;
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 55.5f ) {
                                                    if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                                        return 26.0/431.9;
                                                    } else {
                                                        return 56.0/321.9;
                                                    }
                                                } else {
                                                    return 21.0/475.9;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 61.0/357.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 28.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 11.9921875f ) {
                                if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2397444.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                                    return 63.0/409.9;
                                                } else {
                                                    return 31.0/363.9;
                                                }
                                            } else {
                                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                    return 100.0/607.9;
                                                } else {
                                                    return 151.0/513.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 10.5f ) {
                                                return 108.0/495.9;
                                            } else {
                                                return 125.0/262.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0618311017752f ) {
                                            return 69.0/355.9;
                                        } else {
                                            return 41.0/641.9;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 20.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2261239.0f ) {
                                            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.208333343267f ) {
                                                    return 139.0/409.9;
                                                } else {
                                                    if ( cl->stats.dump_number <= 10.5f ) {
                                                        return 96.0/591.9;
                                                    } else {
                                                        return 98.0/335.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 8.5f ) {
                                                    return 81.0/272.0;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.452173441648f ) {
                                                        return 154.0/228.0;
                                                    } else {
                                                        return 122.0/313.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 6.5f ) {
                                                return 78.0/415.9;
                                            } else {
                                                return 45.0/335.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.929138541222f ) {
                                            if ( rdb0_last_touched_diff <= 24406.5f ) {
                                                return 151.0/258.0;
                                            } else {
                                                return 126.0/367.9;
                                            }
                                        } else {
                                            return 146.0/240.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.831008553505f ) {
                                    return 105.0/238.0;
                                } else {
                                    return 135.0/154.0;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.882394552231f ) {
                                if ( cl->stats.sum_uip1_used <= 27.5f ) {
                                    if ( cl->stats.dump_number <= 52.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 120.0/186.0;
                                        } else {
                                            return 153.0/136.0;
                                        }
                                    } else {
                                        return 174.0/108.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 8066163.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0677629634738f ) {
                                            return 122.0/204.0;
                                        } else {
                                            return 84.0/250.0;
                                        }
                                    } else {
                                        return 78.0/274.0;
                                    }
                                }
                            } else {
                                return 149.0/124.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                        if ( cl->stats.size_rel <= 0.666517078876f ) {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0341252163053f ) {
                                    return 99.0/591.9;
                                } else {
                                    return 121.0/433.9;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 7178302.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.043087657541f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.371210575104f ) {
                                            return 38.0/479.9;
                                        } else {
                                            return 75.0/461.9;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.169265627861f ) {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                return 20.0/483.9;
                                            } else {
                                                return 33.0/415.9;
                                            }
                                        } else {
                                            return 76.0/565.9;
                                        }
                                    }
                                } else {
                                    return 65.0/349.9;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                return 102.0/246.0;
                            } else {
                                return 65.0/421.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                if ( cl->stats.size_rel <= 0.54519790411f ) {
                                    if ( cl->stats.sum_uip1_used <= 34.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            return 34.0/559.9;
                                        } else {
                                            return 83.0/617.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0646066814661f ) {
                                            return 33.0/377.9;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 54.5f ) {
                                                return 21.0/359.9;
                                            } else {
                                                return 21.0/749.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 83.0/469.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 1436.5f ) {
                                    return 6.0/403.9;
                                } else {
                                    return 15.0/379.9;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 106.5f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1137499.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0612306073308f ) {
                                            return 6.0/387.9;
                                        } else {
                                            return 6.0/515.9;
                                        }
                                    } else {
                                        return 30.0/669.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 15.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1956.5f ) {
                                            return 35.0/603.9;
                                        } else {
                                            return 38.0/359.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 24.5f ) {
                                            return 31.0/809.9;
                                        } else {
                                            return 37.0/621.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                    return 23.0/593.9;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.65718126297f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.226473078132f ) {
                                            return 9.0/451.9;
                                        } else {
                                            if ( cl->size() <= 6.5f ) {
                                                return 0.0/731.9;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.392805039883f ) {
                                                    return 5.0/437.9;
                                                } else {
                                                    return 2.0/469.9;
                                                }
                                            }
                                        }
                                    } else {
                                        return 13.0/451.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 10522.0f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                        if ( cl->stats.dump_number <= 29.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 5612.5f ) {
                                    return 7.0/455.9;
                                } else {
                                    return 16.0/421.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.373355805874f ) {
                                    return 34.0/393.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 15740600.0f ) {
                                        return 26.0/379.9;
                                    } else {
                                        return 9.0/603.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 12.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.369897961617f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 27486.0f ) {
                                        if ( cl->stats.size_rel <= 0.257216870785f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.075269497931f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.364964723587f ) {
                                                    return 44.0/605.9;
                                                } else {
                                                    return 45.0/359.9;
                                                }
                                            } else {
                                                return 38.0/607.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 17.5f ) {
                                                if ( cl->stats.size_rel <= 0.328291237354f ) {
                                                    return 7.0/385.9;
                                                } else {
                                                    return 17.0/397.9;
                                                }
                                            } else {
                                                return 40.0/385.9;
                                            }
                                        }
                                    } else {
                                        return 75.0/529.9;
                                    }
                                } else {
                                    return 18.0/451.9;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        return 83.0/341.9;
                                    } else {
                                        return 71.0/393.9;
                                    }
                                } else {
                                    return 27.0/479.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 6282.0f ) {
                            if ( cl->stats.glue <= 6.5f ) {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( rdb0_last_touched_diff <= 2709.5f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 104.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 321.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 1179.0f ) {
                                                    return 8.0/455.9;
                                                } else {
                                                    return 2.0/445.9;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                    return 0.0/831.9;
                                                } else {
                                                    return 5.0/445.9;
                                                }
                                            }
                                        } else {
                                            return 10.0/409.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.278280258179f ) {
                                            return 25.0/393.9;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.170338883996f ) {
                                                return 21.0/401.9;
                                            } else {
                                                return 7.0/739.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 261.0f ) {
                                        return 19.0/603.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0621284395456f ) {
                                            return 13.0/811.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 60877960.0f ) {
                                                return 1.0/377.9;
                                            } else {
                                                return 0.0/637.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 5818.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.446742415428f ) {
                                        return 29.0/609.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0948971584439f ) {
                                            return 17.0/477.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 31190412.0f ) {
                                                return 11.0/579.9;
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.655230641365f ) {
                                                    return 1.0/435.9;
                                                } else {
                                                    return 6.0/707.9;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 25.0/377.9;
                                }
                            }
                        } else {
                            return 23.0/391.9;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 58.5f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0230953041464f ) {
                            return 103.0/501.9;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                if ( rdb0_last_touched_diff <= 19914.5f ) {
                                    return 25.0/683.9;
                                } else {
                                    return 69.0/573.9;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 205.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 10252.0f ) {
                                        return 40.0/309.9;
                                    } else {
                                        return 57.0/294.0;
                                    }
                                } else {
                                    return 39.0/543.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 51175696.0f ) {
                                return 98.0/335.9;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.354184150696f ) {
                                    return 53.0/369.9;
                                } else {
                                    return 21.0/401.9;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.135431349277f ) {
                                return 140.0/355.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 125.0f ) {
                                    return 165.0/323.9;
                                } else {
                                    return 45.0/471.9;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 68806.0f ) {
                if ( cl->stats.size_rel <= 0.680571377277f ) {
                    if ( cl->stats.dump_number <= 11.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 17006.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 71.5f ) {
                                    if ( rdb0_last_touched_diff <= 16298.0f ) {
                                        return 92.0/475.9;
                                    } else {
                                        if ( cl->stats.glue <= 5.5f ) {
                                            return 52.0/315.9;
                                        } else {
                                            return 43.0/411.9;
                                        }
                                    }
                                } else {
                                    return 75.0/264.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.431795626879f ) {
                                    return 12.0/409.9;
                                } else {
                                    return 36.0/361.9;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0428012609482f ) {
                                    return 70.0/319.9;
                                } else {
                                    return 51.0/389.9;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.980000019073f ) {
                                    if ( rdb0_last_touched_diff <= 39529.0f ) {
                                        return 54.0/337.9;
                                    } else {
                                        return 81.0/270.0;
                                    }
                                } else {
                                    return 120.0/216.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 4.5f ) {
                            return 94.0/449.9;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.690666079521f ) {
                                if ( cl->stats.sum_uip1_used <= 50.5f ) {
                                    if ( cl->stats.size_rel <= 0.384368896484f ) {
                                        return 197.0/254.0;
                                    } else {
                                        return 106.0/238.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        return 42.0/369.9;
                                    } else {
                                        return 51.0/327.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.189041480422f ) {
                                    return 107.0/234.0;
                                } else {
                                    return 106.0/214.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.163194447756f ) {
                        if ( cl->stats.sum_uip1_used <= 24.5f ) {
                            return 126.0/234.0;
                        } else {
                            return 73.0/311.9;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 25.5f ) {
                            return 292.0/284.0;
                        } else {
                            return 101.0/299.9;
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 258861.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 3088707.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.01927435398f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 273.0/248.0;
                            } else {
                                if ( cl->size() <= 9.5f ) {
                                    if ( cl->stats.size_rel <= 0.274491846561f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.101840056479f ) {
                                            return 138.0/178.0;
                                        } else {
                                            return 128.0/399.9;
                                        }
                                    } else {
                                        return 151.0/202.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.358851313591f ) {
                                        if ( cl->stats.dump_number <= 21.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0603684671223f ) {
                                                return 108.0/220.0;
                                            } else {
                                                return 136.0/148.0;
                                            }
                                        } else {
                                            return 280.0/158.0;
                                        }
                                    } else {
                                        return 103.0/218.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.107075631618f ) {
                                    return 150.0/138.0;
                                } else {
                                    return 120.0/156.0;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 10.9721870422f ) {
                                    if ( rdb0_last_touched_diff <= 130584.5f ) {
                                        return 151.0/148.0;
                                    } else {
                                        return 170.0/84.0;
                                    }
                                } else {
                                    return 202.0/76.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.222744464874f ) {
                            if ( cl->stats.size_rel <= 0.196419298649f ) {
                                if ( cl->stats.size_rel <= 0.128227055073f ) {
                                    return 94.0/244.0;
                                } else {
                                    return 133.0/178.0;
                                }
                            } else {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.163553118706f ) {
                                        return 70.0/435.9;
                                    } else {
                                        return 79.0/272.0;
                                    }
                                } else {
                                    return 170.0/294.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 63.5f ) {
                                return 182.0/206.0;
                            } else {
                                return 76.0/244.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 40.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 19.5f ) {
                            return 241.0/329.9;
                        } else {
                            if ( rdb0_last_touched_diff <= 376802.5f ) {
                                return 162.0/118.0;
                            } else {
                                return 176.0/180.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 77.5f ) {
                            if ( cl->size() <= 34.5f ) {
                                return 190.0/100.0;
                            } else {
                                return 186.0/58.0;
                            }
                        } else {
                            return 199.0/140.0;
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_4(
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
    if ( cl->stats.sum_uip1_used <= 6.5f ) {
        if ( cl->stats.glue_rel_long <= 0.782184720039f ) {
            if ( rdb0_last_touched_diff <= 81750.5f ) {
                if ( rdb0_last_touched_diff <= 16818.5f ) {
                    if ( cl->stats.dump_number <= 7.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 3375.0f ) {
                                return 151.0/497.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                    return 113.0/445.9;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.108649879694f ) {
                                        return 72.0/397.9;
                                    } else {
                                        return 43.0/361.9;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 6770.0f ) {
                                return 128.0/417.9;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.201091110706f ) {
                                    return 113.0/323.9;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.648824453354f ) {
                                        return 167.0/290.0;
                                    } else {
                                        return 230.0/214.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 14.5f ) {
                            return 139.0/236.0;
                        } else {
                            return 171.0/160.0;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 0.904621064663f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 31614.5f ) {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.75293135643f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                            return 140.0/313.9;
                                        } else {
                                            if ( cl->size() <= 9.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0932487249374f ) {
                                                    return 126.0/415.9;
                                                } else {
                                                    return 86.0/459.9;
                                                }
                                            } else {
                                                return 107.0/262.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.130907595158f ) {
                                            return 210.0/212.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.647736430168f ) {
                                                if ( rdb0_last_touched_diff <= 30817.0f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.27574262023f ) {
                                                        return 122.0/200.0;
                                                    } else {
                                                        return 91.0/220.0;
                                                    }
                                                } else {
                                                    return 128.0/168.0;
                                                }
                                            } else {
                                                return 155.0/202.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 151.0/156.0;
                                }
                            } else {
                                return 265.0/250.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.611871123314f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0496487729251f ) {
                                        if ( cl->stats.dump_number <= 6.5f ) {
                                            return 108.0/210.0;
                                        } else {
                                            return 142.0/144.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 393.5f ) {
                                            return 159.0/246.0;
                                        } else {
                                            return 120.0/299.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 43.5f ) {
                                        return 173.0/118.0;
                                    } else {
                                        return 153.0/174.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 11.5f ) {
                                    return 211.0/315.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.217166751623f ) {
                                        return 207.0/158.0;
                                    } else {
                                        return 184.0/68.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                            return 288.0/70.0;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.497198909521f ) {
                                return 188.0/82.0;
                            } else {
                                return 157.0/124.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.size_rel <= 0.753985106945f ) {
                    if ( cl->size() <= 10.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 176.0/236.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 177740.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 123603.0f ) {
                                        return 203.0/224.0;
                                    } else {
                                        return 125.0/200.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.113005429506f ) {
                                        return 283.0/134.0;
                                    } else {
                                        return 142.0/136.0;
                                    }
                                }
                            }
                        } else {
                            return 295.0/182.0;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.650674700737f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 43.5f ) {
                                if ( cl->stats.glue <= 5.5f ) {
                                    return 181.0/66.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 171384.0f ) {
                                        return 166.0/194.0;
                                    } else {
                                        return 187.0/80.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.123952396214f ) {
                                    return 259.0/42.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 179261.5f ) {
                                        return 223.0/156.0;
                                    } else {
                                        return 229.0/80.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 122611.0f ) {
                                return 166.0/78.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 21.5f ) {
                                    return 198.0/66.0;
                                } else {
                                    return 287.0/36.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 232629.0f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.15405267477f ) {
                            return 183.0/68.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 5.5f ) {
                                return 342.0/50.0;
                            } else {
                                return 337.0/118.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.42027810216f ) {
                            return 329.0/44.0;
                        } else {
                            return 206.0/64.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 967.0f ) {
                if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        return 206.0/327.9;
                    } else {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                            if ( cl->stats.glue <= 9.5f ) {
                                return 144.0/140.0;
                            } else {
                                return 180.0/92.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 1.0210442543f ) {
                                return 235.0/102.0;
                            } else {
                                return 178.0/32.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_long <= 1.07514023781f ) {
                        if ( cl->size() <= 13.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.926576972008f ) {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 199.0/142.0;
                                } else {
                                    return 318.0/108.0;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.864477932453f ) {
                                    return 191.0/86.0;
                                } else {
                                    return 215.0/42.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.330919504166f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.104070216417f ) {
                                    return 282.0/184.0;
                                } else {
                                    if ( cl->stats.size_rel <= 1.02305448055f ) {
                                        if ( cl->size() <= 19.5f ) {
                                            return 240.0/48.0;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.140336081386f ) {
                                                return 218.0/46.0;
                                            } else {
                                                if ( cl->stats.glue <= 14.5f ) {
                                                    return 214.0/64.0;
                                                } else {
                                                    return 185.0/110.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 87.5f ) {
                                            return 257.0/50.0;
                                        } else {
                                            return 210.0/16.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 5523.5f ) {
                                    return 154.0/70.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 252.0/48.0;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                return 351.0/22.0;
                                            } else {
                                                return 267.0/32.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.818092465401f ) {
                                            return 187.0/64.0;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 64409.5f ) {
                                                if ( rdb0_last_touched_diff <= 44121.5f ) {
                                                    return 300.0/94.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 0.961545348167f ) {
                                                        return 164.0/56.0;
                                                    } else {
                                                        return 240.0/30.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 339.0f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 7.25611114502f ) {
                                                            return 220.0/4.0;
                                                        } else {
                                                            return 274.0/44.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 1.90362811089f ) {
                                                            return 199.0/74.0;
                                                        } else {
                                                            if ( cl->stats.glue <= 14.5f ) {
                                                                if ( cl->size() <= 21.5f ) {
                                                                    return 190.0/72.0;
                                                                } else {
                                                                    return 293.0/32.0;
                                                                }
                                                            } else {
                                                                return 194.0/12.0;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                        return 332.0/8.0;
                                                    } else {
                                                        return 204.0/26.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 88.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 9.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 66.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.290814727545f ) {
                                            return 319.0/34.0;
                                        } else {
                                            return 227.0/92.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 75124.5f ) {
                                            return 227.0/18.0;
                                        } else {
                                            return 201.0/48.0;
                                        }
                                    }
                                } else {
                                    return 230.0/24.0;
                                }
                            } else {
                                return 280.0/108.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 178.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.62165307999f ) {
                                    if ( cl->stats.num_antecedents_rel <= 1.11748445034f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            return 235.0/18.0;
                                        } else {
                                            return 340.0/84.0;
                                        }
                                    } else {
                                        return 173.0/72.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 43.5f ) {
                                        if ( cl->stats.dump_number <= 35.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 93615.0f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                            if ( cl->stats.glue_rel_long <= 1.29786467552f ) {
                                                                return 340.0/34.0;
                                                            } else {
                                                                return 322.0/12.0;
                                                            }
                                                        } else {
                                                            if ( rdb0_last_touched_diff <= 46834.0f ) {
                                                                return 220.0/54.0;
                                                            } else {
                                                                return 360.0/42.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( cl->stats.size_rel <= 1.24043405056f ) {
                                                            return 328.0/4.0;
                                                        } else {
                                                            return 193.0/4.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 12.5f ) {
                                                        return 218.0/18.0;
                                                    } else {
                                                        return 190.0/58.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->size() <= 46.5f ) {
                                                    return 260.0/6.0;
                                                } else {
                                                    return 234.0/2.0;
                                                }
                                            }
                                        } else {
                                            return 293.0/58.0;
                                        }
                                    } else {
                                        return 180.0/56.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 40609.0f ) {
                                    if ( cl->size() <= 111.5f ) {
                                        if ( cl->stats.glue <= 27.5f ) {
                                            return 373.0/30.0;
                                        } else {
                                            return 216.0/64.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 221.5f ) {
                                            return 225.0/10.0;
                                        } else {
                                            return 220.0/26.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 51.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->size() <= 87.5f ) {
                                                if ( cl->stats.num_overlap_literals <= 330.5f ) {
                                                    return 356.0/6.0;
                                                } else {
                                                    return 382.0/2.0;
                                                }
                                            } else {
                                                return 1;
                                            }
                                        } else {
                                            if ( rdb0_last_touched_diff <= 187680.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 1.67603814602f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 66.115234375f ) {
                                                        return 213.0/36.0;
                                                    } else {
                                                        return 238.0/28.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 475.5f ) {
                                                        return 216.0/8.0;
                                                    } else {
                                                        return 218.0/18.0;
                                                    }
                                                }
                                            } else {
                                                if ( rdb0_last_touched_diff <= 301286.5f ) {
                                                    return 261.0/6.0;
                                                } else {
                                                    return 183.0/12.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 218.0/36.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                    if ( rdb0_last_touched_diff <= 25781.5f ) {
                        if ( cl->stats.glue <= 10.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.142311245203f ) {
                                return 106.0/307.9;
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                    return 142.0/168.0;
                                } else {
                                    return 154.0/276.0;
                                }
                            }
                        } else {
                            if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                                if ( cl->size() <= 39.5f ) {
                                    return 157.0/110.0;
                                } else {
                                    return 153.0/82.0;
                                }
                            } else {
                                return 241.0/230.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 157.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 64480.5f ) {
                                if ( cl->stats.glue <= 9.5f ) {
                                    return 144.0/248.0;
                                } else {
                                    return 315.0/210.0;
                                }
                            } else {
                                return 256.0/136.0;
                            }
                        } else {
                            if ( cl->size() <= 45.5f ) {
                                return 153.0/74.0;
                            } else {
                                return 211.0/28.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 50295.5f ) {
                        return 177.0/176.0;
                    } else {
                        if ( cl->stats.num_overlap_literals <= 160.5f ) {
                            if ( cl->stats.dump_number <= 35.5f ) {
                                if ( cl->stats.size_rel <= 0.551841378212f ) {
                                    return 164.0/112.0;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.429175257683f ) {
                                        return 320.0/82.0;
                                    } else {
                                        return 254.0/118.0;
                                    }
                                }
                            } else {
                                return 334.0/66.0;
                            }
                        } else {
                            return 281.0/44.0;
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.rdb1_last_touched_diff <= 6085.5f ) {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2504.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0596578493714f ) {
                                            return 52.0/294.0;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 6582052.5f ) {
                                                return 65.0/479.9;
                                            } else {
                                                return 27.0/461.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.486620903015f ) {
                                            if ( cl->stats.dump_number <= 28.5f ) {
                                                return 38.0/411.9;
                                            } else {
                                                return 62.0/278.0;
                                            }
                                        } else {
                                            return 87.0/407.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 44.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.65715098381f ) {
                                            return 121.0/525.9;
                                        } else {
                                            return 182.0/453.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                            return 57.0/311.9;
                                        } else {
                                            return 48.0/447.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 2009.0f ) {
                                        return 62.0/329.9;
                                    } else {
                                        return 50.0/323.9;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                        return 13.0/581.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.138480275869f ) {
                                            return 48.0/411.9;
                                        } else {
                                            return 40.0/675.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0316498056054f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0511663332582f ) {
                                        if ( cl->stats.glue_rel_long <= 0.286627084017f ) {
                                            return 27.0/373.9;
                                        } else {
                                            return 17.0/559.9;
                                        }
                                    } else {
                                        return 34.0/355.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.448311626911f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.157577067614f ) {
                                            return 14.0/653.9;
                                        } else {
                                            return 1.0/491.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1873.5f ) {
                                            return 28.0/349.9;
                                        } else {
                                            return 17.0/399.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->size() <= 6.5f ) {
                                    return 24.0/643.9;
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.361417651176f ) {
                                        return 43.0/701.9;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 4287765.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.996527791023f ) {
                                                    return 70.0/439.9;
                                                } else {
                                                    return 62.0/260.0;
                                                }
                                            } else {
                                                return 40.0/533.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 4.5f ) {
                                                return 50.0/455.9;
                                            } else {
                                                if ( cl->size() <= 22.5f ) {
                                                    return 37.0/541.9;
                                                } else {
                                                    return 22.0/543.9;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 5.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0334893837571f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    return 48.0/397.9;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 44.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 541.0f ) {
                                            return 10.0/731.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0252311360091f ) {
                                                return 16.0/391.9;
                                            } else {
                                                return 10.0/489.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 84.5f ) {
                                            return 26.0/375.9;
                                        } else {
                                            return 15.0/489.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 310.5f ) {
                                    if ( rdb0_last_touched_diff <= 5917.0f ) {
                                        if ( cl->stats.used_for_uip_creation <= 19.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 39.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0950728729367f ) {
                                                    if ( cl->stats.glue_rel_long <= 0.292495310307f ) {
                                                        return 22.0/427.9;
                                                    } else {
                                                        if ( cl->stats.sum_uip1_used <= 77.5f ) {
                                                            return 3.0/495.9;
                                                        } else {
                                                            return 15.0/669.9;
                                                        }
                                                    }
                                                } else {
                                                    return 20.0/399.9;
                                                }
                                            } else {
                                                return 22.0/361.9;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.232063531876f ) {
                                                return 5.0/775.9;
                                            } else {
                                                return 8.0/459.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                            return 15.0/407.9;
                                        } else {
                                            return 47.0/321.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.479893356562f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1084.0f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 65.5f ) {
                                                if ( cl->stats.dump_number <= 34.5f ) {
                                                    return 0.0/513.9;
                                                } else {
                                                    return 1.0/425.9;
                                                }
                                            } else {
                                                return 3.0/405.9;
                                            }
                                        } else {
                                            return 8.0/771.9;
                                        }
                                    } else {
                                        return 6.0/421.9;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 3145.5f ) {
                                if ( cl->stats.sum_uip1_used <= 113.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 18.0/621.9;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0839002281427f ) {
                                            return 14.0/453.9;
                                        } else {
                                            return 56.0/701.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 30.5f ) {
                                        return 21.0/401.9;
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 20.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.495235085487f ) {
                                                return 22.0/373.9;
                                            } else {
                                                return 6.0/639.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 58.5f ) {
                                                if ( rdb0_last_touched_diff <= 732.0f ) {
                                                    if ( cl->stats.sum_uip1_used <= 653.5f ) {
                                                        return 0.0/833.9;
                                                    } else {
                                                        return 3.0/479.9;
                                                    }
                                                } else {
                                                    return 12.0/575.9;
                                                }
                                            } else {
                                                return 8.0/363.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 52.5f ) {
                                    if ( rdb0_last_touched_diff <= 9902.0f ) {
                                        return 59.0/399.9;
                                    } else {
                                        return 78.0/359.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 42024200.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.117356777191f ) {
                                            return 59.0/379.9;
                                        } else {
                                            return 39.0/745.9;
                                        }
                                    } else {
                                        return 11.0/479.9;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.612685382366f ) {
                        if ( rdb0_last_touched_diff <= 5790.0f ) {
                            if ( cl->stats.num_overlap_literals <= 14.5f ) {
                                if ( cl->stats.sum_uip1_used <= 67.5f ) {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 7.5f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 30.0/431.9;
                                            } else {
                                                return 43.0/437.9;
                                            }
                                        } else {
                                            return 24.0/507.9;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.107915788889f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2191999.0f ) {
                                                return 77.0/256.0;
                                            } else {
                                                return 65.0/409.9;
                                            }
                                        } else {
                                            return 70.0/535.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 273.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0855762585998f ) {
                                            return 55.0/549.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1664.5f ) {
                                                return 20.0/691.9;
                                            } else {
                                                return 42.0/591.9;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 2141.5f ) {
                                            return 12.0/685.9;
                                        } else {
                                            return 14.0/391.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.439850509167f ) {
                                        return 51.0/345.9;
                                    } else {
                                        return 36.0/371.9;
                                    }
                                } else {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 85.0/331.9;
                                    } else {
                                        return 53.0/381.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.dump_number <= 72.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0438682734966f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.382295489311f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.266396045685f ) {
                                                return 127.0/425.9;
                                            } else {
                                                return 85.0/379.9;
                                            }
                                        } else {
                                            return 90.0/629.9;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 59.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 979469.0f ) {
                                                return 56.0/345.9;
                                            } else {
                                                return 69.0/254.0;
                                            }
                                        } else {
                                            return 20.0/473.9;
                                        }
                                    }
                                } else {
                                    return 53.0/579.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.108346268535f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0341892540455f ) {
                                        return 98.0/296.0;
                                    } else {
                                        return 112.0/236.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.523169577122f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.273315757513f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1707197.0f ) {
                                                return 83.0/319.9;
                                            } else {
                                                return 65.0/515.9;
                                            }
                                        } else {
                                            return 73.0/303.9;
                                        }
                                    } else {
                                        return 142.0/431.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.136228054762f ) {
                                if ( rdb0_last_touched_diff <= 20782.0f ) {
                                    return 53.0/367.9;
                                } else {
                                    return 98.0/393.9;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.448979616165f ) {
                                    if ( rdb0_last_touched_diff <= 25493.5f ) {
                                        return 59.0/359.9;
                                    } else {
                                        return 110.0/256.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 22.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 77.5f ) {
                                            return 125.0/272.0;
                                        } else {
                                            return 170.0/216.0;
                                        }
                                    } else {
                                        return 140.0/489.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                if ( rdb0_last_touched_diff <= 5363.0f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 23739.5f ) {
                                        return 111.0/445.9;
                                    } else {
                                        return 155.0/369.9;
                                    }
                                } else {
                                    return 122.0/238.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 48.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.834493994713f ) {
                                        return 78.0/521.9;
                                    } else {
                                        return 73.0/280.0;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                            return 34.0/401.9;
                                        } else {
                                            return 55.0/349.9;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.525254130363f ) {
                                            return 14.0/611.9;
                                        } else {
                                            return 30.0/407.9;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.sum_uip1_used <= 25.5f ) {
                        if ( cl->stats.size_rel <= 0.394938468933f ) {
                            if ( cl->stats.glue_rel_long <= 0.463214337826f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 464282.0f ) {
                                    return 71.0/375.9;
                                } else {
                                    return 120.0/226.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 512609.0f ) {
                                    return 102.0/282.0;
                                } else {
                                    return 120.0/154.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 48.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 27667.0f ) {
                                    return 164.0/385.9;
                                } else {
                                    return 139.0/176.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 19566.5f ) {
                                    return 104.0/180.0;
                                } else {
                                    return 296.0/206.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 44.5f ) {
                            if ( cl->stats.sum_uip1_used <= 95.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 7022925.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.448229581118f ) {
                                        if ( cl->stats.glue_rel_long <= 0.330336272717f ) {
                                            return 59.0/278.0;
                                        } else {
                                            return 50.0/389.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 27638.0f ) {
                                            return 91.0/475.9;
                                        } else {
                                            return 106.0/266.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.112793087959f ) {
                                        return 161.0/335.9;
                                    } else {
                                        return 80.0/250.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue <= 6.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 238.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 24139326.0f ) {
                                            return 41.0/355.9;
                                        } else {
                                            return 62.0/298.0;
                                        }
                                    } else {
                                        return 38.0/595.9;
                                    }
                                } else {
                                    return 85.0/473.9;
                                }
                            }
                        } else {
                            return 138.0/252.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 27.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            return 136.0/385.9;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                return 59.0/469.9;
                            } else {
                                return 100.0/357.9;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                    return 42.0/523.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0577350221574f ) {
                                        return 47.0/301.9;
                                    } else {
                                        return 35.0/385.9;
                                    }
                                }
                            } else {
                                return 56.0/286.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 38.0/457.9;
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.102612033486f ) {
                                    return 31.0/469.9;
                                } else {
                                    return 17.0/625.9;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.dump_number <= 13.5f ) {
                if ( cl->stats.sum_uip1_used <= 21.5f ) {
                    if ( rdb0_last_touched_diff <= 28488.5f ) {
                        if ( cl->stats.size_rel <= 0.600527763367f ) {
                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                    return 73.0/319.9;
                                } else {
                                    return 103.0/226.0;
                                }
                            } else {
                                return 94.0/479.9;
                            }
                        } else {
                            return 119.0/224.0;
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 36.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.605313420296f ) {
                                if ( cl->stats.dump_number <= 8.5f ) {
                                    return 78.0/313.9;
                                } else {
                                    return 109.0/224.0;
                                }
                            } else {
                                return 128.0/202.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.10570788383f ) {
                                return 198.0/361.9;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 53.5f ) {
                                    return 163.0/126.0;
                                } else {
                                    return 197.0/68.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 42.5f ) {
                        if ( cl->stats.size_rel <= 0.402204990387f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.14161773026f ) {
                                return 77.0/363.9;
                            } else {
                                return 36.0/353.9;
                            }
                        } else {
                            return 140.0/421.9;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 100.5f ) {
                            if ( cl->stats.dump_number <= 7.5f ) {
                                return 48.0/567.9;
                            } else {
                                return 90.0/459.9;
                            }
                        } else {
                            return 36.0/643.9;
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 3087054.0f ) {
                    if ( cl->stats.dump_number <= 30.5f ) {
                        if ( cl->stats.glue <= 9.5f ) {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0318418964744f ) {
                                    return 176.0/190.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.513705849648f ) {
                                        return 152.0/483.9;
                                    } else {
                                        return 168.0/246.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.720305085182f ) {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 653380.0f ) {
                                            return 152.0/154.0;
                                        } else {
                                            return 117.0/192.0;
                                        }
                                    } else {
                                        return 186.0/150.0;
                                    }
                                } else {
                                    return 122.0/196.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 10.8194446564f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 574541.0f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.249262273312f ) {
                                        return 131.0/114.0;
                                    } else {
                                        return 192.0/104.0;
                                    }
                                } else {
                                    return 174.0/276.0;
                                }
                            } else {
                                return 222.0/94.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.162166982889f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 439814.5f ) {
                                return 267.0/144.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                    if ( rdb0_last_touched_diff <= 220628.5f ) {
                                        return 221.0/212.0;
                                    } else {
                                        return 162.0/240.0;
                                    }
                                } else {
                                    return 169.0/102.0;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.762524843216f ) {
                                if ( cl->stats.dump_number <= 45.5f ) {
                                    return 144.0/124.0;
                                } else {
                                    return 153.0/84.0;
                                }
                            } else {
                                return 267.0/70.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                        if ( cl->stats.dump_number <= 61.5f ) {
                            if ( cl->stats.dump_number <= 23.5f ) {
                                return 47.0/509.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                    return 136.0/166.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 113697.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 59063.5f ) {
                                            return 53.0/353.9;
                                        } else {
                                            return 74.0/323.9;
                                        }
                                    } else {
                                        return 126.0/365.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 94.5f ) {
                                return 249.0/226.0;
                            } else {
                                return 173.0/427.9;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 69.5f ) {
                            if ( rdb0_last_touched_diff <= 124715.5f ) {
                                return 185.0/274.0;
                            } else {
                                return 223.0/222.0;
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.588359832764f ) {
                                if ( cl->stats.glue_rel_long <= 0.572828173637f ) {
                                    return 156.0/292.0;
                                } else {
                                    return 78.0/258.0;
                                }
                            } else {
                                return 72.0/288.0;
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_5(
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
    if ( cl->stats.sum_delta_confl_uip1_used <= 2984.5f ) {
        if ( cl->stats.glue <= 8.5f ) {
            if ( cl->stats.rdb1_last_touched_diff <= 70021.5f ) {
                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                    if ( cl->stats.num_overlap_literals_rel <= 0.100418783724f ) {
                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                return 112.0/455.9;
                            } else {
                                return 120.0/250.0;
                            }
                        } else {
                            return 47.0/403.9;
                        }
                    } else {
                        if ( cl->size() <= 9.5f ) {
                            return 119.0/445.9;
                        } else {
                            if ( rdb0_last_touched_diff <= 16817.5f ) {
                                return 128.0/152.0;
                            } else {
                                return 216.0/162.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 11.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                            return 62.0/288.0;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.236111104488f ) {
                                if ( cl->size() <= 8.5f ) {
                                    return 171.0/391.9;
                                } else {
                                    return 197.0/228.0;
                                }
                            } else {
                                return 161.0/162.0;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0468252189457f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                return 149.0/216.0;
                            } else {
                                return 173.0/184.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 86.5f ) {
                                if ( cl->stats.size_rel <= 0.454348742962f ) {
                                    return 197.0/282.0;
                                } else {
                                    return 272.0/154.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.01524615288f ) {
                                    return 253.0/82.0;
                                } else {
                                    return 188.0/136.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 5.5f ) {
                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                            return 125.0/176.0;
                        } else {
                            return 179.0/158.0;
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            return 220.0/218.0;
                        } else {
                            return 231.0/86.0;
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 25.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                            return 154.0/124.0;
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 103.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.00632238108665f ) {
                                    return 300.0/152.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                        return 343.0/62.0;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.697625160217f ) {
                                            return 241.0/118.0;
                                        } else {
                                            return 215.0/56.0;
                                        }
                                    }
                                }
                            } else {
                                return 178.0/144.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 355251.0f ) {
                            return 264.0/62.0;
                        } else {
                            if ( cl->stats.size_rel <= 0.585831046104f ) {
                                return 209.0/18.0;
                            } else {
                                return 192.0/28.0;
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 75.5f ) {
                if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 22685.0f ) {
                        return 216.0/224.0;
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.945267677307f ) {
                            if ( rdb0_last_touched_diff <= 128744.5f ) {
                                return 260.0/142.0;
                            } else {
                                return 286.0/86.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 81032.0f ) {
                                return 228.0/62.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 37.5f ) {
                                    return 227.0/20.0;
                                } else {
                                    return 180.0/42.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.0568268895149f ) {
                        if ( cl->stats.num_overlap_literals_rel <= 0.0284214429557f ) {
                            if ( cl->stats.glue_rel_queue <= 1.02857613564f ) {
                                return 185.0/66.0;
                            } else {
                                return 209.0/28.0;
                            }
                        } else {
                            return 238.0/172.0;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.818816423416f ) {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.487856328487f ) {
                                    return 164.0/50.0;
                                } else {
                                    return 135.0/136.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 4.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                        if ( rdb0_last_touched_diff <= 132469.5f ) {
                                            return 248.0/90.0;
                                        } else {
                                            return 281.0/38.0;
                                        }
                                    } else {
                                        return 317.0/34.0;
                                    }
                                } else {
                                    return 331.0/154.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 38.2335586548f ) {
                                if ( cl->size() <= 32.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.31302082539f ) {
                                        if ( cl->size() <= 29.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 12552.0f ) {
                                                return 352.0/92.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.07125234604f ) {
                                                    if ( cl->stats.glue <= 12.5f ) {
                                                        if ( cl->stats.num_total_lits_antecedents <= 75.0f ) {
                                                            return 218.0/14.0;
                                                        } else {
                                                            return 216.0/52.0;
                                                        }
                                                    } else {
                                                        return 243.0/88.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 249151.0f ) {
                                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                            if ( cl->stats.antecedents_glue_long_reds_var <= 23.9617347717f ) {
                                                                if ( cl->stats.num_antecedents_rel <= 0.488638699055f ) {
                                                                    return 261.0/8.0;
                                                                } else {
                                                                    if ( cl->stats.num_overlap_literals <= 164.5f ) {
                                                                        if ( rdb0_last_touched_diff <= 49792.5f ) {
                                                                            return 255.0/48.0;
                                                                        } else {
                                                                            if ( cl->stats.num_antecedents_rel <= 0.900790452957f ) {
                                                                                return 195.0/18.0;
                                                                            } else {
                                                                                return 222.0/10.0;
                                                                            }
                                                                        }
                                                                    } else {
                                                                        return 395.0/26.0;
                                                                    }
                                                                }
                                                            } else {
                                                                return 185.0/40.0;
                                                            }
                                                        } else {
                                                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                                                if ( cl->stats.antec_num_total_lits_rel <= 1.32589483261f ) {
                                                                    return 296.0/64.0;
                                                                } else {
                                                                    return 189.0/60.0;
                                                                }
                                                            } else {
                                                                return 243.0/10.0;
                                                            }
                                                        }
                                                    } else {
                                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                                            return 229.0/22.0;
                                                        } else {
                                                            return 257.0/72.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 297.0/110.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 73.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 46.5f ) {
                                                return 225.0/20.0;
                                            } else {
                                                return 194.0/42.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue <= 14.5f ) {
                                                return 216.0/26.0;
                                            } else {
                                                return 421.0/10.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 41393.0f ) {
                                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.49093568325f ) {
                                                return 190.0/44.0;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.887016415596f ) {
                                                    return 317.0/18.0;
                                                } else {
                                                    return 365.0/68.0;
                                                }
                                            }
                                        } else {
                                            return 194.0/62.0;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 84487.5f ) {
                                                if ( cl->stats.dump_number <= 6.5f ) {
                                                    return 238.0/6.0;
                                                } else {
                                                    return 217.0/36.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 135058.0f ) {
                                                    return 1;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.10028982162f ) {
                                                        return 204.0/10.0;
                                                    } else {
                                                        return 1;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.798238396645f ) {
                                                return 208.0/40.0;
                                            } else {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                    if ( rdb0_last_touched_diff <= 150634.0f ) {
                                                        return 257.0/18.0;
                                                    } else {
                                                        return 221.0/58.0;
                                                    }
                                                } else {
                                                    if ( cl->size() <= 49.5f ) {
                                                        return 270.0/4.0;
                                                    } else {
                                                        if ( cl->stats.num_total_lits_antecedents <= 302.5f ) {
                                                            return 253.0/28.0;
                                                        } else {
                                                            return 207.0/8.0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.980582952499f ) {
                                    return 227.0/34.0;
                                } else {
                                    if ( cl->stats.size_rel <= 1.14950489998f ) {
                                        if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                            if ( cl->stats.dump_number <= 3.5f ) {
                                                return 239.0/20.0;
                                            } else {
                                                return 323.0/10.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 383.5f ) {
                                                if ( cl->stats.glue <= 29.5f ) {
                                                    return 196.0/18.0;
                                                } else {
                                                    return 193.0/34.0;
                                                }
                                            } else {
                                                return 374.0/26.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.232103645802f ) {
                                            return 1;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.781216561794f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                    return 218.0/10.0;
                                                } else {
                                                    return 205.0/28.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.41163229942f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 1.62195944786f ) {
                                                        return 298.0/2.0;
                                                    } else {
                                                        return 395.0/24.0;
                                                    }
                                                } else {
                                                    return 393.0/4.0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_last_touched_diff <= 47935.5f ) {
                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.801234602928f ) {
                        return 142.0/278.0;
                    } else {
                        if ( cl->stats.glue_rel_long <= 1.02434766293f ) {
                            if ( cl->stats.glue_rel_queue <= 0.781929373741f ) {
                                return 129.0/166.0;
                            } else {
                                return 238.0/190.0;
                            }
                        } else {
                            return 284.0/74.0;
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.582491636276f ) {
                        if ( cl->size() <= 68.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 316.0f ) {
                                return 287.0/106.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 169133.0f ) {
                                    return 246.0/212.0;
                                } else {
                                    return 170.0/64.0;
                                }
                            }
                        } else {
                            return 202.0/42.0;
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 16.2421875f ) {
                            return 273.0/80.0;
                        } else {
                            return 327.0/18.0;
                        }
                    }
                }
            }
        }
    } else {
        if ( rdb0_last_touched_diff <= 27548.5f ) {
            if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 2083417.5f ) {
                        if ( cl->stats.sum_uip1_used <= 12.5f ) {
                            if ( cl->stats.size_rel <= 0.654844403267f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 63936.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.254342973232f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.699650347233f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 5252.5f ) {
                                                return 48.0/413.9;
                                            } else {
                                                return 73.0/373.9;
                                            }
                                        } else {
                                            return 74.0/264.0;
                                        }
                                    } else {
                                        return 107.0/252.0;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 62039.0f ) {
                                        if ( cl->size() <= 7.5f ) {
                                            return 110.0/607.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0850850939751f ) {
                                                return 126.0/162.0;
                                            } else {
                                                if ( cl->stats.num_total_lits_antecedents <= 53.5f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.562853753567f ) {
                                                        return 86.0/254.0;
                                                    } else {
                                                        return 136.0/303.9;
                                                    }
                                                } else {
                                                    return 179.0/258.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 136.0/166.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.611666679382f ) {
                                    return 97.0/206.0;
                                } else {
                                    if ( cl->stats.glue <= 13.5f ) {
                                        if ( cl->stats.dump_number <= 6.5f ) {
                                            return 98.0/232.0;
                                        } else {
                                            return 163.0/134.0;
                                        }
                                    } else {
                                        return 167.0/120.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 60.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 24548.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.324999988079f ) {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0630479902029f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 5050.0f ) {
                                                    return 59.0/315.9;
                                                } else {
                                                    return 48.0/325.9;
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.16770452261f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0959003120661f ) {
                                                        return 36.0/379.9;
                                                    } else {
                                                        return 23.0/419.9;
                                                    }
                                                } else {
                                                    return 81.0/691.9;
                                                }
                                            }
                                        } else {
                                            return 79.0/303.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6986.0f ) {
                                            return 116.0/443.9;
                                        } else {
                                            return 45.0/294.0;
                                        }
                                    }
                                } else {
                                    return 102.0/333.9;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.222506642342f ) {
                                    return 123.0/319.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 579667.0f ) {
                                        return 76.0/276.0;
                                    } else {
                                        return 61.0/284.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 11.5f ) {
                            if ( cl->stats.sum_uip1_used <= 65.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.6180986166f ) {
                                    if ( cl->stats.dump_number <= 21.5f ) {
                                        return 40.0/455.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 42.5f ) {
                                            return 127.0/423.9;
                                        } else {
                                            return 122.0/323.9;
                                        }
                                    }
                                } else {
                                    return 73.0/465.9;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 28.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 13.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0288017019629f ) {
                                            return 14.0/391.9;
                                        } else {
                                            return 10.0/409.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.342819929123f ) {
                                            return 33.0/381.9;
                                        } else {
                                            return 41.0/673.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 22985288.0f ) {
                                        return 95.0/435.9;
                                    } else {
                                        if ( rdb0_last_touched_diff <= 11529.5f ) {
                                            if ( cl->stats.dump_number <= 64.5f ) {
                                                return 21.0/645.9;
                                            } else {
                                                if ( cl->stats.sum_uip1_used <= 287.5f ) {
                                                    return 42.0/419.9;
                                                } else {
                                                    return 15.0/419.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                return 63.0/485.9;
                                            } else {
                                                return 27.0/409.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.602496087551f ) {
                                if ( cl->stats.dump_number <= 28.5f ) {
                                    return 64.0/705.9;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 112.5f ) {
                                        return 98.0/214.0;
                                    } else {
                                        return 52.0/387.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 12097.5f ) {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 7245412.5f ) {
                                            return 76.0/248.0;
                                        } else {
                                            return 60.0/359.9;
                                        }
                                    } else {
                                        return 72.0/473.9;
                                    }
                                } else {
                                    return 121.0/357.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 38.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 4446.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.614334762096f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                            if ( cl->size() <= 6.5f ) {
                                                return 19.0/361.9;
                                            } else {
                                                return 60.0/415.9;
                                            }
                                        } else {
                                            return 16.0/397.9;
                                        }
                                    } else {
                                        return 82.0/571.9;
                                    }
                                } else {
                                    if ( cl->size() <= 12.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 12163.0f ) {
                                            return 76.0/627.9;
                                        } else {
                                            return 86.0/477.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 73.5f ) {
                                            return 66.0/371.9;
                                        } else {
                                            return 84.0/280.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.311022102833f ) {
                                    return 124.0/339.9;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.278973281384f ) {
                                        return 64.0/469.9;
                                    } else {
                                        return 84.0/268.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 10.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.546000540257f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.124647408724f ) {
                                        return 46.0/397.9;
                                    } else {
                                        return 22.0/479.9;
                                    }
                                } else {
                                    return 14.0/413.9;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 65.5f ) {
                                    return 81.0/523.9;
                                } else {
                                    return 25.0/423.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                            if ( rdb0_last_touched_diff <= 3094.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.205620378256f ) {
                                    return 49.0/355.9;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.454201638699f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0399378538132f ) {
                                                return 26.0/499.9;
                                            } else {
                                                return 13.0/555.9;
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 36.5f ) {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.185878038406f ) {
                                                    return 21.0/507.9;
                                                } else {
                                                    return 28.0/341.9;
                                                }
                                            } else {
                                                return 49.0/397.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 74.5f ) {
                                            return 35.0/581.9;
                                        } else {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 24.0/513.9;
                                            } else {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.0484689809382f ) {
                                                    return 9.0/399.9;
                                                } else {
                                                    return 6.0/671.9;
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 9.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 7433.0f ) {
                                        return 33.0/575.9;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 16850.0f ) {
                                            return 44.0/383.9;
                                        } else {
                                            return 31.0/395.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.583921432495f ) {
                                        return 45.0/473.9;
                                    } else {
                                        return 63.0/357.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1696.5f ) {
                                if ( rdb0_last_touched_diff <= 408.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 312.5f ) {
                                            return 3.0/499.9;
                                        } else {
                                            return 0.0/407.9;
                                        }
                                    } else {
                                        if ( cl->size() <= 10.5f ) {
                                            if ( cl->stats.dump_number <= 19.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.0868507996202f ) {
                                                    return 7.0/455.9;
                                                } else {
                                                    return 3.0/539.9;
                                                }
                                            } else {
                                                return 0.0/435.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 595.0f ) {
                                                if ( cl->size() <= 27.5f ) {
                                                    return 8.0/481.9;
                                                } else {
                                                    return 5.0/695.9;
                                                }
                                            } else {
                                                return 16.0/585.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 150.0f ) {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 4305521.5f ) {
                                                return 18.0/379.9;
                                            } else {
                                                return 38.0/333.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 473.0f ) {
                                                if ( cl->size() <= 7.5f ) {
                                                    return 18.0/445.9;
                                                } else {
                                                    return 38.0/527.9;
                                                }
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 10.0/661.9;
                                                } else {
                                                    return 16.0/403.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 1916.0f ) {
                                            if ( cl->stats.size_rel <= 0.0600487366319f ) {
                                                return 18.0/459.9;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 6149.0f ) {
                                                    if ( rdb0_last_touched_diff <= 898.5f ) {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 252.5f ) {
                                                            return 13.0/405.9;
                                                        } else {
                                                            return 9.0/565.9;
                                                        }
                                                    } else {
                                                        if ( cl->size() <= 11.5f ) {
                                                            if ( cl->stats.sum_uip1_used <= 268.0f ) {
                                                                return 5.0/569.9;
                                                            } else {
                                                                if ( cl->stats.num_overlap_literals <= 4.5f ) {
                                                                    return 1.0/493.9;
                                                                } else {
                                                                    return 0.0/539.9;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->stats.dump_number <= 19.5f ) {
                                                                return 8.0/461.9;
                                                            } else {
                                                                return 13.0/423.9;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    return 14.0/375.9;
                                                }
                                            }
                                        } else {
                                            return 18.0/387.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0189074315131f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0224667713046f ) {
                                        return 44.0/735.9;
                                    } else {
                                        return 62.0/559.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 5983.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.115911990404f ) {
                                            if ( cl->stats.glue_rel_long <= 0.260177075863f ) {
                                                return 22.0/525.9;
                                            } else {
                                                if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                    return 21.0/465.9;
                                                } else {
                                                    if ( cl->stats.sum_uip1_used <= 264.5f ) {
                                                        if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                            return 8.0/793.9;
                                                        } else {
                                                            return 16.0/443.9;
                                                        }
                                                    } else {
                                                        return 2.0/693.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                                return 8.0/451.9;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.359792709351f ) {
                                                    return 19.0/467.9;
                                                } else {
                                                    return 47.0/545.9;
                                                }
                                            }
                                        }
                                    } else {
                                        return 42.0/399.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 1073469.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.592300653458f ) {
                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 7442.0f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0424569025636f ) {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 37.0/355.9;
                                    } else {
                                        return 78.0/327.9;
                                    }
                                } else {
                                    return 39.0/567.9;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                    return 84.0/250.0;
                                } else {
                                    return 80.0/511.9;
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.317191809416f ) {
                                return 96.0/525.9;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 5662.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                        return 83.0/242.0;
                                    } else {
                                        return 50.0/355.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 199670.0f ) {
                                        return 126.0/413.9;
                                    } else {
                                        return 133.0/220.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 8.5f ) {
                            if ( cl->stats.num_overlap_literals <= 20.5f ) {
                                if ( cl->stats.dump_number <= 5.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.694885134697f ) {
                                        return 75.0/244.0;
                                    } else {
                                        return 68.0/419.9;
                                    }
                                } else {
                                    return 151.0/361.9;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.478671342134f ) {
                                    return 155.0/202.0;
                                } else {
                                    return 82.0/218.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.247448980808f ) {
                                if ( rdb0_last_touched_diff <= 13242.5f ) {
                                    return 61.0/274.0;
                                } else {
                                    return 102.0/288.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.glue_rel_long <= 0.923454523087f ) {
                                        return 165.0/180.0;
                                    } else {
                                        return 238.0/156.0;
                                    }
                                } else {
                                    return 170.0/319.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.494897961617f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 5946.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 6076468.5f ) {
                                    if ( cl->stats.dump_number <= 11.5f ) {
                                        return 20.0/377.9;
                                    } else {
                                        return 86.0/339.9;
                                    }
                                } else {
                                    return 52.0/685.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.110962882638f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0354690104723f ) {
                                        return 101.0/405.9;
                                    } else {
                                        return 45.0/427.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 10442.0f ) {
                                        return 106.0/367.9;
                                    } else {
                                        return 99.0/469.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 34.5f ) {
                                return 115.0/184.0;
                            } else {
                                return 89.0/585.9;
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0863223150373f ) {
                            if ( rdb0_last_touched_diff <= 6639.5f ) {
                                return 29.0/719.9;
                            } else {
                                return 40.0/379.9;
                            }
                        } else {
                            return 47.0/445.9;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                if ( cl->stats.sum_uip1_used <= 23.5f ) {
                    if ( cl->stats.glue <= 15.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.dump_number <= 21.5f ) {
                                if ( cl->stats.size_rel <= 0.117258906364f ) {
                                    return 77.0/272.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 100.0/284.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0493827164173f ) {
                                            return 139.0/315.9;
                                        } else {
                                            return 122.0/186.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 32.5f ) {
                                    return 132.0/148.0;
                                } else {
                                    return 119.0/168.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 208673.5f ) {
                                if ( cl->stats.num_overlap_literals <= 71.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 150.0/367.9;
                                    } else {
                                        if ( cl->size() <= 14.5f ) {
                                            return 126.0/206.0;
                                        } else {
                                            return 190.0/224.0;
                                        }
                                    }
                                } else {
                                    return 167.0/144.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                    return 219.0/317.9;
                                } else {
                                    if ( rdb0_last_touched_diff <= 43291.0f ) {
                                        return 152.0/160.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                            return 189.0/84.0;
                                        } else {
                                            return 159.0/104.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        return 301.0/170.0;
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 55916.0f ) {
                        if ( cl->size() <= 17.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 54201276.0f ) {
                                    if ( cl->stats.dump_number <= 38.5f ) {
                                        if ( cl->stats.size_rel <= 0.246474727988f ) {
                                            return 100.0/533.9;
                                        } else {
                                            return 57.0/551.9;
                                        }
                                    } else {
                                        return 138.0/437.9;
                                    }
                                } else {
                                    return 27.0/441.9;
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.153601050377f ) {
                                    return 83.0/226.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 32980.5f ) {
                                        return 77.0/555.9;
                                    } else {
                                        return 65.0/270.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 57.5f ) {
                                return 112.0/274.0;
                            } else {
                                return 95.0/435.9;
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                            if ( cl->stats.size_rel <= 0.299795925617f ) {
                                return 90.0/230.0;
                            } else {
                                return 68.0/327.9;
                            }
                        } else {
                            return 162.0/303.9;
                        }
                    }
                }
            } else {
                if ( cl->stats.num_total_lits_antecedents <= 33.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 3089010.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 64329.5f ) {
                            if ( cl->stats.sum_uip1_used <= 14.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 131615.0f ) {
                                        return 137.0/284.0;
                                    } else {
                                        return 133.0/134.0;
                                    }
                                } else {
                                    return 177.0/373.9;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                    return 96.0/282.0;
                                } else {
                                    return 106.0/537.9;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 11.5f ) {
                                if ( cl->stats.size_rel <= 0.391972273588f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 48892.5f ) {
                                        return 188.0/204.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.381587058306f ) {
                                            return 204.0/86.0;
                                        } else {
                                            return 277.0/234.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                        return 217.0/64.0;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 162254.0f ) {
                                            return 156.0/122.0;
                                        } else {
                                            return 177.0/116.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 39.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        return 123.0/325.9;
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 701189.0f ) {
                                            return 217.0/299.9;
                                        } else {
                                            return 134.0/262.0;
                                        }
                                    }
                                } else {
                                    return 211.0/188.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 87860.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 7765041.0f ) {
                                return 96.0/301.9;
                            } else {
                                return 78.0/501.9;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 44.5f ) {
                                return 207.0/164.0;
                            } else {
                                if ( cl->stats.size_rel <= 0.186867788434f ) {
                                    return 171.0/313.9;
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        return 65.0/371.9;
                                    } else {
                                        return 106.0/305.9;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 80023.0f ) {
                        if ( cl->stats.sum_uip1_used <= 13.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 279.0f ) {
                                if ( cl->stats.dump_number <= 13.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 4.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.30499982834f ) {
                                            return 192.0/184.0;
                                        } else {
                                            return 201.0/108.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.726479351521f ) {
                                            return 154.0/296.0;
                                        } else {
                                            return 159.0/164.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 22.5f ) {
                                        return 187.0/122.0;
                                    } else {
                                        return 181.0/66.0;
                                    }
                                }
                            } else {
                                return 264.0/58.0;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.468532979488f ) {
                                if ( cl->stats.dump_number <= 14.5f ) {
                                    return 80.0/471.9;
                                } else {
                                    return 158.0/347.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 50392.5f ) {
                                    return 86.0/210.0;
                                } else {
                                    return 194.0/238.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.400020420551f ) {
                            if ( cl->size() <= 9.5f ) {
                                return 162.0/319.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 16.5f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 63.5f ) {
                                        return 168.0/86.0;
                                    } else {
                                        return 175.0/68.0;
                                    }
                                } else {
                                    return 208.0/238.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 152.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 252172.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                        if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                            return 196.0/104.0;
                                        } else {
                                            return 173.0/230.0;
                                        }
                                    } else {
                                        if ( cl->size() <= 17.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                                return 172.0/114.0;
                                            } else {
                                                return 137.0/162.0;
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 527688.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                    return 196.0/90.0;
                                                } else {
                                                    return 307.0/72.0;
                                                }
                                            } else {
                                                return 201.0/146.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                        if ( cl->stats.size_rel <= 0.934659004211f ) {
                                            if ( cl->stats.size_rel <= 0.62884414196f ) {
                                                return 177.0/62.0;
                                            } else {
                                                return 181.0/106.0;
                                            }
                                        } else {
                                            return 184.0/40.0;
                                        }
                                    } else {
                                        return 173.0/114.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 1.77565550804f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.57156008482f ) {
                                        return 228.0/110.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 8.513671875f ) {
                                            return 178.0/54.0;
                                        } else {
                                            return 199.0/36.0;
                                        }
                                    }
                                } else {
                                    return 199.0/122.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_6(
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
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                if ( cl->stats.sum_uip1_used <= 3.5f ) {
                    if ( cl->stats.glue <= 10.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.287848830223f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 6985.0f ) {
                                    return 54.0/278.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                        return 77.0/266.0;
                                    } else {
                                        return 96.0/238.0;
                                    }
                                }
                            } else {
                                return 173.0/262.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 32.5f ) {
                                return 101.0/258.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.76108288765f ) {
                                    if ( cl->stats.size_rel <= 0.436326146126f ) {
                                        return 116.0/196.0;
                                    } else {
                                        return 187.0/208.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 5.17374992371f ) {
                                        return 152.0/142.0;
                                    } else {
                                        return 132.0/126.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.814885854721f ) {
                            return 154.0/148.0;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 28730.0f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.696035385132f ) {
                                        return 187.0/56.0;
                                    } else {
                                        return 327.0/22.0;
                                    }
                                } else {
                                    return 322.0/26.0;
                                }
                            } else {
                                return 330.0/150.0;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 10.5f ) {
                        if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.243329331279f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0165282394737f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 11.5f ) {
                                        return 55.0/367.9;
                                    } else {
                                        return 81.0/280.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 59.5f ) {
                                        if ( rdb0_last_touched_diff <= 10033.0f ) {
                                            return 51.0/437.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1593477.5f ) {
                                                if ( cl->size() <= 8.5f ) {
                                                    if ( rdb0_last_touched_diff <= 22986.0f ) {
                                                        return 44.0/339.9;
                                                    } else {
                                                        return 71.0/369.9;
                                                    }
                                                } else {
                                                    return 81.0/264.0;
                                                }
                                            } else {
                                                return 119.0/397.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.19684971869f ) {
                                            return 63.0/545.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 35604984.0f ) {
                                                return 25.0/529.9;
                                            } else {
                                                return 36.0/401.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.494897961617f ) {
                                    if ( rdb0_last_touched_diff <= 19213.0f ) {
                                        return 70.0/298.0;
                                    } else {
                                        return 82.0/248.0;
                                    }
                                } else {
                                    return 61.0/317.9;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 4588.5f ) {
                                if ( cl->size() <= 6.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0119501184672f ) {
                                        return 48.0/321.9;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.262129545212f ) {
                                                    return 40.0/387.9;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0551896095276f ) {
                                                        if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                                            return 36.0/441.9;
                                                        } else {
                                                            return 17.0/407.9;
                                                        }
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals_rel <= 0.0924534648657f ) {
                                                            return 8.0/449.9;
                                                        } else {
                                                            return 23.0/463.9;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 10.0/507.9;
                                            }
                                        } else {
                                            return 71.0/515.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 28648.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 414457.0f ) {
                                            return 57.0/451.9;
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.177349865437f ) {
                                                if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                    return 16.0/443.9;
                                                } else {
                                                    return 40.0/595.9;
                                                }
                                            } else {
                                                return 46.0/541.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.174852639437f ) {
                                            return 55.0/282.0;
                                        } else {
                                            return 68.0/288.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 66.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0464647784829f ) {
                                        return 144.0/443.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.105268202722f ) {
                                            return 41.0/391.9;
                                        } else {
                                            return 80.0/379.9;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 6134.5f ) {
                                        return 19.0/403.9;
                                    } else {
                                        return 43.0/487.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals_rel <= 0.236039936543f ) {
                            if ( rdb0_last_touched_diff <= 6015.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.dump_number <= 15.5f ) {
                                            return 41.0/319.9;
                                        } else {
                                            return 66.0/323.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 40720.5f ) {
                                            return 90.0/311.9;
                                        } else {
                                            return 110.0/204.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.425205260515f ) {
                                        if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                            return 59.0/294.0;
                                        } else {
                                            return 31.0/423.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.743652582169f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0645584911108f ) {
                                                return 35.0/665.9;
                                            } else {
                                                return 53.0/681.9;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.209923699498f ) {
                                                return 63.0/401.9;
                                            } else {
                                                return 34.0/423.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 28.5f ) {
                                    if ( cl->stats.size_rel <= 0.814882576466f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 6642.5f ) {
                                            return 109.0/319.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 17.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 659997.5f ) {
                                                    return 150.0/453.9;
                                                } else {
                                                    return 113.0/192.0;
                                                }
                                            } else {
                                                return 215.0/278.0;
                                            }
                                        }
                                    } else {
                                        return 163.0/192.0;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                                        if ( rdb0_last_touched_diff <= 23918.0f ) {
                                            return 68.0/495.9;
                                        } else {
                                            return 58.0/323.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 23.5f ) {
                                            return 155.0/423.9;
                                        } else {
                                            return 90.0/387.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.715640187263f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 33069.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 15.5f ) {
                                        return 88.0/260.0;
                                    } else {
                                        return 90.0/537.9;
                                    }
                                } else {
                                    return 85.0/252.0;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.00452506542f ) {
                                        if ( cl->size() <= 23.5f ) {
                                            return 155.0/365.9;
                                        } else {
                                            return 133.0/196.0;
                                        }
                                    } else {
                                        return 158.0/202.0;
                                    }
                                } else {
                                    return 118.0/617.9;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->stats.antec_num_total_lits_rel <= 0.429225862026f ) {
                        if ( cl->stats.sum_uip1_used <= 37.5f ) {
                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                if ( rdb0_last_touched_diff <= 11737.5f ) {
                                    return 86.0/405.9;
                                } else {
                                    return 152.0/343.9;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1172370.5f ) {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 61.0/425.9;
                                    } else {
                                        return 68.0/365.9;
                                    }
                                } else {
                                    return 93.0/363.9;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 1153.0f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.117612689734f ) {
                                    return 18.0/539.9;
                                } else {
                                    return 21.0/343.9;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0606674551964f ) {
                                    if ( cl->stats.size_rel <= 0.17843735218f ) {
                                        return 70.0/611.9;
                                    } else {
                                        return 125.0/601.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.488440304995f ) {
                                        return 26.0/551.9;
                                    } else {
                                        return 67.0/601.9;
                                    }
                                }
                            }
                        }
                    } else {
                        return 157.0/429.9;
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 239363.0f ) {
                        if ( cl->stats.glue_rel_long <= 0.762152135372f ) {
                            if ( rdb0_last_touched_diff <= 4723.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.110607206821f ) {
                                    if ( rdb0_last_touched_diff <= 962.5f ) {
                                        return 14.0/427.9;
                                    } else {
                                        return 30.0/429.9;
                                    }
                                } else {
                                    return 38.0/403.9;
                                }
                            } else {
                                return 40.0/329.9;
                            }
                        } else {
                            return 80.0/278.0;
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 2548.0f ) {
                            if ( cl->stats.glue_rel_queue <= 0.550074100494f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1355.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 28.5f ) {
                                        if ( rdb0_last_touched_diff <= 500.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.383085489273f ) {
                                                if ( cl->stats.size_rel <= 0.138328969479f ) {
                                                    return 3.0/415.9;
                                                } else {
                                                    return 8.0/429.9;
                                                }
                                            } else {
                                                return 2.0/417.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 16.5f ) {
                                                if ( cl->stats.used_for_uip_creation <= 11.5f ) {
                                                    return 10.0/401.9;
                                                } else {
                                                    return 3.0/467.9;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 0.326830297709f ) {
                                                    return 26.0/413.9;
                                                } else {
                                                    return 13.0/529.9;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.0361986532807f ) {
                                            return 16.0/733.9;
                                        } else {
                                            if ( cl->stats.used_for_uip_creation <= 112.5f ) {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 41.5f ) {
                                                    if ( rdb0_last_touched_diff <= 493.5f ) {
                                                        return 2.0/425.9;
                                                    } else {
                                                        return 5.0/391.9;
                                                    }
                                                } else {
                                                    return 1.0/835.9;
                                                }
                                            } else {
                                                return 6.0/529.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0761111155152f ) {
                                        if ( cl->stats.size_rel <= 0.185113668442f ) {
                                            if ( cl->stats.sum_uip1_used <= 120.5f ) {
                                                return 34.0/603.9;
                                            } else {
                                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                    return 6.0/541.9;
                                                } else {
                                                    return 21.0/589.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.dump_number <= 11.5f ) {
                                                return 20.0/739.9;
                                            } else {
                                                return 3.0/727.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 13.5f ) {
                                            return 35.0/541.9;
                                        } else {
                                            return 15.0/363.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 6.5f ) {
                                    if ( cl->size() <= 13.5f ) {
                                        return 25.0/601.9;
                                    } else {
                                        return 53.0/599.9;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.584201693535f ) {
                                        return 27.0/393.9;
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                                            return 19.0/389.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 1671.5f ) {
                                                if ( cl->stats.sum_uip1_used <= 97.5f ) {
                                                    return 17.0/661.9;
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                                        return 0.0/495.9;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 0.695130586624f ) {
                                                            return 9.0/395.9;
                                                        } else {
                                                            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                                                return 8.0/473.9;
                                                            } else {
                                                                return 1.0/551.9;
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 1.0/443.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 121.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0793969333172f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.468340158463f ) {
                                            return 24.0/587.9;
                                        } else {
                                            return 28.0/373.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0425879918039f ) {
                                            return 41.0/331.9;
                                        } else {
                                            return 24.0/419.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 6.5f ) {
                                        return 45.0/589.9;
                                    } else {
                                        return 79.0/563.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.00847205705941f ) {
                                    if ( cl->stats.size_rel <= 0.0972614735365f ) {
                                        return 35.0/393.9;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0537199676037f ) {
                                            return 20.0/355.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.122054457664f ) {
                                                if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                                    return 20.0/477.9;
                                                } else {
                                                    return 5.0/409.9;
                                                }
                                            } else {
                                                return 8.0/777.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 291.5f ) {
                                        return 15.0/557.9;
                                    } else {
                                        return 5.0/431.9;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 4.75771617889f ) {
                if ( cl->stats.sum_uip1_used <= 11.5f ) {
                    if ( cl->stats.num_overlap_literals <= 22.5f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                return 119.0/401.9;
                            } else {
                                if ( rdb0_last_touched_diff <= 31217.0f ) {
                                    if ( cl->stats.size_rel <= 0.413750678301f ) {
                                        return 150.0/517.9;
                                    } else {
                                        return 100.0/214.0;
                                    }
                                } else {
                                    return 218.0/258.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 7.5f ) {
                                if ( rdb0_last_touched_diff <= 16195.5f ) {
                                    return 115.0/375.9;
                                } else {
                                    if ( cl->stats.size_rel <= 0.55035674572f ) {
                                        return 128.0/218.0;
                                    } else {
                                        return 178.0/126.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_antecedents_rel <= 0.189869552851f ) {
                                    return 152.0/100.0;
                                } else {
                                    return 146.0/204.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.852779507637f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 212.0/333.9;
                            } else {
                                if ( cl->stats.glue <= 7.5f ) {
                                    return 164.0/152.0;
                                } else {
                                    return 152.0/76.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 1894.0f ) {
                                return 310.0/78.0;
                            } else {
                                return 184.0/184.0;
                            }
                        }
                    }
                } else {
                    if ( rdb0_last_touched_diff <= 66627.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0790104269981f ) {
                            if ( rdb0_last_touched_diff <= 16586.0f ) {
                                if ( cl->stats.glue_rel_long <= 0.801041662693f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                        if ( cl->size() <= 10.5f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0891268849373f ) {
                                                return 74.0/573.9;
                                            } else {
                                                return 34.0/501.9;
                                            }
                                        } else {
                                            return 67.0/313.9;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 81.5f ) {
                                            return 50.0/627.9;
                                        } else {
                                            return 18.0/729.9;
                                        }
                                    }
                                } else {
                                    return 71.0/391.9;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.548948287964f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                        if ( cl->stats.size_rel <= 0.195130765438f ) {
                                            if ( cl->stats.sum_uip1_used <= 89.0f ) {
                                                return 87.0/313.9;
                                            } else {
                                                return 52.0/319.9;
                                            }
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.114693783224f ) {
                                                return 53.0/381.9;
                                            } else {
                                                return 42.0/355.9;
                                            }
                                        }
                                    } else {
                                        return 120.0/399.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 42.5f ) {
                                        return 101.0/238.0;
                                    } else {
                                        return 71.0/363.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.357884198427f ) {
                                if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                        return 177.0/399.9;
                                    } else {
                                        return 119.0/463.9;
                                    }
                                } else {
                                    if ( cl->size() <= 13.5f ) {
                                        return 58.0/701.9;
                                    } else {
                                        return 61.0/335.9;
                                    }
                                }
                            } else {
                                return 165.0/449.9;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 92377.0f ) {
                            return 147.0/186.0;
                        } else {
                            return 113.0/234.0;
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 886.5f ) {
                    if ( cl->stats.glue <= 10.5f ) {
                        return 202.0/146.0;
                    } else {
                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                            if ( cl->stats.size_rel <= 1.2693375349f ) {
                                if ( rdb0_last_touched_diff <= 33062.5f ) {
                                    return 247.0/48.0;
                                } else {
                                    return 317.0/22.0;
                                }
                            } else {
                                return 343.0/14.0;
                            }
                        } else {
                            return 201.0/42.0;
                        }
                    }
                } else {
                    if ( cl->stats.size_rel <= 1.11878728867f ) {
                        if ( cl->stats.glue_rel_queue <= 0.825620114803f ) {
                            if ( cl->stats.sum_uip1_used <= 20.5f ) {
                                return 160.0/250.0;
                            } else {
                                return 69.0/315.9;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 267319.0f ) {
                                return 149.0/150.0;
                            } else {
                                return 113.0/184.0;
                            }
                        }
                    } else {
                        return 234.0/138.0;
                    }
                }
            }
        }
    } else {
        if ( cl->size() <= 10.5f ) {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.sum_delta_confl_uip1_used <= 48.0f ) {
                    if ( cl->size() <= 8.5f ) {
                        return 196.0/361.9;
                    } else {
                        return 175.0/120.0;
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.sum_uip1_used <= 12.5f ) {
                            if ( cl->stats.dump_number <= 6.5f ) {
                                return 115.0/561.9;
                            } else {
                                return 147.0/226.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 26575.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.569805204868f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 7556.5f ) {
                                        return 32.0/485.9;
                                    } else {
                                        return 50.0/379.9;
                                    }
                                } else {
                                    return 50.0/286.0;
                                }
                            } else {
                                return 102.0/367.9;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 27.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 64335.0f ) {
                                if ( rdb0_last_touched_diff <= 45020.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 93.0/276.0;
                                    } else {
                                        return 112.0/268.0;
                                    }
                                } else {
                                    return 124.0/250.0;
                                }
                            } else {
                                return 202.0/178.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 6187947.5f ) {
                                return 85.0/475.9;
                            } else {
                                return 147.0/487.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_uip1_used <= 9.5f ) {
                    if ( rdb0_last_touched_diff <= 83730.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                            if ( cl->stats.dump_number <= 4.5f ) {
                                return 139.0/401.9;
                            } else {
                                return 220.0/341.9;
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 1.30500006676f ) {
                                return 218.0/264.0;
                            } else {
                                return 153.0/76.0;
                            }
                        }
                    } else {
                        if ( cl->size() <= 8.5f ) {
                            if ( cl->size() <= 4.5f ) {
                                return 163.0/252.0;
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.066716298461f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        return 147.0/122.0;
                                    } else {
                                        return 182.0/210.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.306071341038f ) {
                                        if ( rdb0_last_touched_diff <= 182570.5f ) {
                                            return 127.0/140.0;
                                        } else {
                                            return 144.0/108.0;
                                        }
                                    } else {
                                        return 233.0/112.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                if ( cl->stats.glue <= 6.5f ) {
                                    return 167.0/90.0;
                                } else {
                                    return 172.0/122.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 62.0f ) {
                                    return 200.0/52.0;
                                } else {
                                    return 166.0/96.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 63805.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                            if ( cl->stats.sum_uip1_used <= 30.5f ) {
                                return 84.0/256.0;
                            } else {
                                return 61.0/309.9;
                            }
                        } else {
                            return 45.0/399.9;
                        }
                    } else {
                        if ( cl->stats.num_overlap_literals <= 3.5f ) {
                            return 155.0/361.9;
                        } else {
                            if ( cl->stats.sum_uip1_used <= 54.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0643761307001f ) {
                                    return 256.0/256.0;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                        return 165.0/190.0;
                                    } else {
                                        return 186.0/371.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 228890.0f ) {
                                    if ( rdb0_last_touched_diff <= 138933.0f ) {
                                        return 85.0/270.0;
                                    } else {
                                        return 84.0/282.0;
                                    }
                                } else {
                                    return 128.0/222.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.antecedents_glue_long_reds_var <= 0.262687891722f ) {
                if ( cl->stats.sum_uip1_used <= 6.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 20113.0f ) {
                        if ( cl->stats.glue_rel_long <= 0.652919530869f ) {
                            return 121.0/351.9;
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.299315333366f ) {
                                return 150.0/172.0;
                            } else {
                                return 148.0/132.0;
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 7.5f ) {
                            if ( cl->stats.glue_rel_queue <= 1.34344911575f ) {
                                if ( rdb0_last_touched_diff <= 176895.5f ) {
                                    if ( rdb0_last_touched_diff <= 51261.0f ) {
                                        return 186.0/86.0;
                                    } else {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            return 246.0/70.0;
                                        } else {
                                            return 406.0/162.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 12.5f ) {
                                        if ( rdb0_last_touched_diff <= 298723.0f ) {
                                            return 242.0/24.0;
                                        } else {
                                            return 200.0/30.0;
                                        }
                                    } else {
                                        return 182.0/56.0;
                                    }
                                }
                            } else {
                                return 256.0/12.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 46517.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.699852108955f ) {
                                    return 129.0/174.0;
                                } else {
                                    return 166.0/124.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 18.5f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.386422634125f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.583238840103f ) {
                                            return 141.0/118.0;
                                        } else {
                                            return 326.0/114.0;
                                        }
                                    } else {
                                        return 185.0/144.0;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 137937.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.876405596733f ) {
                                            return 345.0/158.0;
                                        } else {
                                            return 189.0/36.0;
                                        }
                                    } else {
                                        return 298.0/58.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 35.5f ) {
                        if ( cl->stats.num_antecedents_rel <= 0.380138993263f ) {
                            if ( rdb0_last_touched_diff <= 110219.0f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 52621.0f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0990610420704f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0354115143418f ) {
                                            return 149.0/309.9;
                                        } else {
                                            return 87.0/337.9;
                                        }
                                    } else {
                                        return 121.0/170.0;
                                    }
                                } else {
                                    return 187.0/224.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 263735.0f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        return 137.0/118.0;
                                    } else {
                                        return 192.0/104.0;
                                    }
                                } else {
                                    return 228.0/56.0;
                                }
                            }
                        } else {
                            if ( cl->stats.size_rel <= 0.630049884319f ) {
                                return 85.0/270.0;
                            } else {
                                return 129.0/182.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.0980917811394f ) {
                            return 175.0/421.9;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 115272.5f ) {
                                if ( cl->stats.dump_number <= 22.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2607914.0f ) {
                                        return 36.0/325.9;
                                    } else {
                                        return 19.0/461.9;
                                    }
                                } else {
                                    return 81.0/204.0;
                                }
                            } else {
                                return 180.0/290.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue <= 9.5f ) {
                    if ( cl->stats.glue_rel_queue <= 0.798562645912f ) {
                        if ( rdb0_last_touched_diff <= 47124.0f ) {
                            if ( rdb0_last_touched_diff <= 25959.5f ) {
                                return 161.0/345.9;
                            } else {
                                return 259.0/296.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.673277020454f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 43.5f ) {
                                    if ( rdb0_last_touched_diff <= 135019.5f ) {
                                        return 220.0/262.0;
                                    } else {
                                        return 300.0/152.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 34.5f ) {
                                        return 294.0/126.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 95.5f ) {
                                            return 148.0/132.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                return 173.0/90.0;
                                            } else {
                                                return 187.0/78.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.936587810516f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.315546691418f ) {
                                        return 208.0/92.0;
                                    } else {
                                        return 160.0/124.0;
                                    }
                                } else {
                                    return 215.0/22.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 4.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.784744977951f ) {
                                if ( cl->size() <= 18.5f ) {
                                    if ( rdb0_last_touched_diff <= 94798.5f ) {
                                        return 238.0/114.0;
                                    } else {
                                        return 279.0/48.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.324884355068f ) {
                                        return 198.0/36.0;
                                    } else {
                                        return 207.0/14.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 2.73527765274f ) {
                                    return 166.0/98.0;
                                } else {
                                    return 333.0/120.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 72164.5f ) {
                                return 125.0/190.0;
                            } else {
                                return 189.0/118.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.436780929565f ) {
                        if ( cl->stats.sum_uip1_used <= 10.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.size_rel <= 1.17511296272f ) {
                                    if ( rdb0_last_touched_diff <= 28401.5f ) {
                                        return 309.0/224.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.22222828865f ) {
                                                if ( cl->stats.num_overlap_literals <= 26.5f ) {
                                                    return 297.0/86.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 104567.0f ) {
                                                        return 275.0/56.0;
                                                    } else {
                                                        return 280.0/24.0;
                                                    }
                                                }
                                            } else {
                                                return 275.0/22.0;
                                            }
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.730841517448f ) {
                                                return 235.0/160.0;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.256596446037f ) {
                                                    return 285.0/64.0;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.708770275116f ) {
                                                        return 161.0/82.0;
                                                    } else {
                                                        return 289.0/94.0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 316.0/60.0;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                            if ( cl->size() <= 49.5f ) {
                                                return 192.0/12.0;
                                            } else {
                                                return 1;
                                            }
                                        } else {
                                            return 294.0/46.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.10152328014f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 19.5f ) {
                                        return 223.0/18.0;
                                    } else {
                                        return 202.0/54.0;
                                    }
                                } else {
                                    return 334.0/6.0;
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 156.0/355.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 31.5f ) {
                                    return 275.0/180.0;
                                } else {
                                    return 125.0/204.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.932626307011f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                return 275.0/164.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 1.08037114143f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.650125384331f ) {
                                        return 254.0/88.0;
                                    } else {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                            return 210.0/24.0;
                                        } else {
                                            return 275.0/56.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 333.5f ) {
                                        return 175.0/96.0;
                                    } else {
                                        return 252.0/70.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.sum_uip1_used <= 10.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 7.57269382477f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.344414711f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 559.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.898245573044f ) {
                                                    return 380.0/76.0;
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                                        return 223.0/74.0;
                                                    } else {
                                                        return 225.0/130.0;
                                                    }
                                                }
                                            } else {
                                                return 188.0/12.0;
                                            }
                                        } else {
                                            return 330.0/40.0;
                                        }
                                    } else {
                                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                                            if ( cl->stats.glue_rel_long <= 1.11092042923f ) {
                                                if ( cl->stats.glue <= 18.5f ) {
                                                    if ( cl->stats.num_overlap_literals <= 97.5f ) {
                                                        return 249.0/76.0;
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 1.45725655556f ) {
                                                            return 256.0/14.0;
                                                        } else {
                                                            return 217.0/46.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 1.84217762947f ) {
                                                        if ( rdb0_last_touched_diff <= 73971.0f ) {
                                                            return 214.0/36.0;
                                                        } else {
                                                            return 206.0/14.0;
                                                        }
                                                    } else {
                                                        return 219.0/12.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.317378461361f ) {
                                                    return 192.0/30.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_long <= 1.18139362335f ) {
                                                        if ( cl->size() <= 42.5f ) {
                                                            return 265.0/14.0;
                                                        } else {
                                                            return 223.0/2.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.dump_number <= 11.5f ) {
                                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                                if ( cl->stats.num_total_lits_antecedents <= 228.5f ) {
                                                                    return 212.0/16.0;
                                                                } else {
                                                                    return 339.0/90.0;
                                                                }
                                                            } else {
                                                                if ( cl->stats.glue_rel_queue <= 1.35275900364f ) {
                                                                    return 309.0/10.0;
                                                                } else {
                                                                    return 265.0/30.0;
                                                                }
                                                            }
                                                        } else {
                                                            if ( cl->size() <= 40.5f ) {
                                                                return 373.0/12.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.938751578331f ) {
                                                    return 229.0/18.0;
                                                } else {
                                                    return 200.0/28.0;
                                                }
                                            } else {
                                                if ( cl->size() <= 77.5f ) {
                                                    if ( cl->stats.antecedents_glue_long_reds_var <= 25.0200004578f ) {
                                                        return 292.0/64.0;
                                                    } else {
                                                        return 232.0/24.0;
                                                    }
                                                } else {
                                                    return 274.0/82.0;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    return 134.0/178.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 158.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 1.07436072826f ) {
                                        return 238.0/40.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 154513.5f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 384.5f ) {
                                                return 243.0/2.0;
                                            } else {
                                                return 194.0/12.0;
                                            }
                                        } else {
                                            if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                                return 264.0/34.0;
                                            } else {
                                                return 316.0/8.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 204.0/42.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_7(
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
    if ( rdb0_last_touched_diff <= 25866.5f ) {
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.used_for_uip_creation <= 2.5f ) {
                if ( cl->size() <= 10.5f ) {
                    if ( cl->size() <= 6.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.18570843339f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0637770593166f ) {
                                if ( cl->stats.sum_uip1_used <= 78.5f ) {
                                    if ( cl->size() <= 5.5f ) {
                                        if ( rdb0_last_touched_diff <= 6407.5f ) {
                                            return 67.0/489.9;
                                        } else {
                                            if ( cl->stats.dump_number <= 7.5f ) {
                                                return 50.0/317.9;
                                            } else {
                                                return 96.0/286.0;
                                            }
                                        }
                                    } else {
                                        return 43.0/331.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 62619004.0f ) {
                                        return 59.0/679.9;
                                    } else {
                                        return 19.0/351.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.342950046062f ) {
                                        return 71.0/659.9;
                                    } else {
                                        if ( cl->stats.dump_number <= 20.5f ) {
                                            return 22.0/693.9;
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                return 43.0/367.9;
                                            } else {
                                                return 22.0/391.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 8406891.0f ) {
                                        return 72.0/467.9;
                                    } else {
                                        return 27.0/399.9;
                                    }
                                }
                            }
                        } else {
                            return 90.0/509.9;
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.305677592754f ) {
                            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 93.0/425.9;
                                } else {
                                    return 116.0/234.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0184551551938f ) {
                                    return 82.0/333.9;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 26492260.0f ) {
                                        if ( rdb0_last_touched_diff <= 16970.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                                if ( cl->stats.rdb1_last_touched_diff <= 21756.5f ) {
                                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                                        return 72.0/491.9;
                                                    } else {
                                                        if ( cl->stats.antec_num_total_lits_rel <= 0.116688199341f ) {
                                                            return 18.0/507.9;
                                                        } else {
                                                            return 36.0/457.9;
                                                        }
                                                    }
                                                } else {
                                                    return 57.0/266.0;
                                                }
                                            } else {
                                                return 80.0/347.9;
                                            }
                                        } else {
                                            return 81.0/403.9;
                                        }
                                    } else {
                                        return 44.0/645.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 5076.5f ) {
                                return 59.0/375.9;
                            } else {
                                return 150.0/397.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals_rel <= 0.312718391418f ) {
                        if ( cl->size() <= 50.5f ) {
                            if ( cl->stats.sum_uip1_used <= 24.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.986821532249f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 11150.5f ) {
                                            return 108.0/162.0;
                                        } else {
                                            if ( cl->stats.dump_number <= 17.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 314635.5f ) {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.172852605581f ) {
                                                        return 111.0/258.0;
                                                    } else {
                                                        return 113.0/224.0;
                                                    }
                                                } else {
                                                    return 68.0/244.0;
                                                }
                                            } else {
                                                return 174.0/232.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.499807059765f ) {
                                            return 60.0/274.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.73844063282f ) {
                                                return 112.0/339.9;
                                            } else {
                                                return 103.0/214.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 175.0/224.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.136531472206f ) {
                                        if ( rdb0_last_touched_diff <= 4719.0f ) {
                                            return 82.0/234.0;
                                        } else {
                                            return 109.0/557.9;
                                        }
                                    } else {
                                        return 42.0/323.9;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                        return 37.0/433.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 58.5f ) {
                                            return 70.0/383.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.161842927337f ) {
                                                return 52.0/383.9;
                                            } else {
                                                return 17.0/347.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 10150.0f ) {
                                return 124.0/395.9;
                            } else {
                                return 183.0/218.0;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.95529872179f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 142.5f ) {
                                if ( cl->stats.dump_number <= 7.5f ) {
                                    return 162.0/286.0;
                                } else {
                                    return 127.0/351.9;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.77483022213f ) {
                                    return 116.0/214.0;
                                } else {
                                    return 207.0/192.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 3.5f ) {
                                return 359.0/90.0;
                            } else {
                                return 178.0/365.9;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.num_overlap_literals <= 14.5f ) {
                        if ( cl->stats.sum_uip1_used <= 59.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 6979.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 643871.0f ) {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        return 34.0/377.9;
                                    } else {
                                        return 29.0/515.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 2159.5f ) {
                                        return 28.0/375.9;
                                    } else {
                                        return 54.0/359.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                    return 91.0/359.9;
                                } else {
                                    if ( cl->stats.size_rel <= 0.246723681688f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0319191813469f ) {
                                            return 78.0/405.9;
                                        } else {
                                            return 50.0/337.9;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.125959932804f ) {
                                            return 50.0/357.9;
                                        } else {
                                            return 53.0/675.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 336.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0129486927763f ) {
                                    return 42.0/375.9;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                        if ( cl->stats.used_for_uip_creation <= 9.5f ) {
                                            return 27.0/703.9;
                                        } else {
                                            return 8.0/417.9;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0734110027552f ) {
                                            return 36.0/335.9;
                                        } else {
                                            if ( cl->stats.dump_number <= 35.5f ) {
                                                if ( cl->stats.glue_rel_long <= 0.460306614637f ) {
                                                    return 17.0/337.9;
                                                } else {
                                                    return 15.0/519.9;
                                                }
                                            } else {
                                                return 29.0/317.9;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0923213660717f ) {
                                    return 28.0/619.9;
                                } else {
                                    return 6.0/471.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 6.54444408417f ) {
                                if ( cl->size() <= 7.5f ) {
                                    return 24.0/379.9;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 2492697.5f ) {
                                            return 102.0/521.9;
                                        } else {
                                            return 34.0/371.9;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 9.5f ) {
                                            return 80.0/258.0;
                                        } else {
                                            return 75.0/375.9;
                                        }
                                    }
                                }
                            } else {
                                return 102.0/359.9;
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.162432342768f ) {
                                return 39.0/397.9;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.669590353966f ) {
                                    return 32.0/521.9;
                                } else {
                                    return 10.0/381.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 1144562.0f ) {
                        if ( cl->stats.sum_uip1_used <= 38.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.123618379235f ) {
                                if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 23.5f ) {
                                        return 20.0/371.9;
                                    } else {
                                        return 5.0/431.9;
                                    }
                                } else {
                                    return 72.0/657.9;
                                }
                            } else {
                                return 79.0/559.9;
                            }
                        } else {
                            if ( cl->stats.glue_rel_long <= 0.628989875317f ) {
                                if ( rdb0_last_touched_diff <= 2238.5f ) {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0282940678298f ) {
                                        return 14.0/375.9;
                                    } else {
                                        return 7.0/699.9;
                                    }
                                } else {
                                    return 23.0/349.9;
                                }
                            } else {
                                return 33.0/453.9;
                            }
                        }
                    } else {
                        if ( cl->stats.used_for_uip_creation <= 12.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.0629071593285f ) {
                                if ( cl->stats.sum_uip1_used <= 133.5f ) {
                                    return 52.0/465.9;
                                } else {
                                    return 32.0/771.9;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 12088368.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 38.5f ) {
                                        return 36.0/351.9;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.247448980808f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.150451987982f ) {
                                                return 27.0/635.9;
                                            } else {
                                                return 10.0/707.9;
                                            }
                                        } else {
                                            return 37.0/507.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.296638786793f ) {
                                        if ( cl->stats.glue <= 6.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.443837225437f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 44163712.0f ) {
                                                    return 7.0/455.9;
                                                } else {
                                                    return 25.0/579.9;
                                                }
                                            } else {
                                                return 5.0/505.9;
                                            }
                                        } else {
                                            return 34.0/553.9;
                                        }
                                    } else {
                                        return 3.0/473.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.13066893816f ) {
                                if ( rdb0_last_touched_diff <= 1535.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 6.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.191978365183f ) {
                                            return 1.0/549.9;
                                        } else {
                                            if ( cl->stats.num_total_lits_antecedents <= 18.5f ) {
                                                if ( cl->stats.dump_number <= 5.5f ) {
                                                    return 7.0/457.9;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.136430472136f ) {
                                                        return 4.0/647.9;
                                                    } else {
                                                        return 0.0/793.9;
                                                    }
                                                }
                                            } else {
                                                return 10.0/411.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.1372705549f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 14186668.0f ) {
                                                return 10.0/611.9;
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                    return 17.0/425.9;
                                                } else {
                                                    return 9.0/433.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.127593249083f ) {
                                                return 6.0/407.9;
                                            } else {
                                                return 3.0/787.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 35.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0892382338643f ) {
                                            if ( cl->stats.sum_uip1_used <= 336.5f ) {
                                                return 16.0/549.9;
                                            } else {
                                                return 24.0/349.9;
                                            }
                                        } else {
                                            return 11.0/703.9;
                                        }
                                    } else {
                                        return 6.0/463.9;
                                    }
                                }
                            } else {
                                if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 304.5f ) {
                                        return 12.0/397.9;
                                    } else {
                                        return 8.0/569.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 1158.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 1275.0f ) {
                                            return 8.0/611.9;
                                        } else {
                                            return 12.0/409.9;
                                        }
                                    } else {
                                        return 33.0/565.9;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.num_overlap_literals <= 82.5f ) {
                if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                    if ( cl->size() <= 9.5f ) {
                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.669939756393f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 10787.5f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0567920580506f ) {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0659916996956f ) {
                                                return 103.0/535.9;
                                            } else {
                                                return 84.0/298.0;
                                            }
                                        } else {
                                            return 59.0/435.9;
                                        }
                                    } else {
                                        return 55.0/627.9;
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 12.5f ) {
                                        return 69.0/439.9;
                                    } else {
                                        return 99.0/244.0;
                                    }
                                }
                            } else {
                                return 96.0/341.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0684725791216f ) {
                                    return 108.0/188.0;
                                } else {
                                    if ( cl->stats.size_rel <= 0.276050686836f ) {
                                        return 118.0/515.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                            return 120.0/212.0;
                                        } else {
                                            return 84.0/264.0;
                                        }
                                    }
                                }
                            } else {
                                return 45.0/401.9;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.809661388397f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 663.0f ) {
                                if ( cl->stats.glue_rel_queue <= 0.557628512383f ) {
                                    return 106.0/182.0;
                                } else {
                                    return 225.0/184.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 13549.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 337070.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.680602967739f ) {
                                            if ( cl->stats.sum_uip1_used <= 5.5f ) {
                                                return 119.0/266.0;
                                            } else {
                                                return 124.0/471.9;
                                            }
                                        } else {
                                            return 113.0/222.0;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            return 30.0/393.9;
                                        } else {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.113866537809f ) {
                                                if ( cl->size() <= 20.5f ) {
                                                    return 75.0/301.9;
                                                } else {
                                                    return 98.0/246.0;
                                                }
                                            } else {
                                                return 60.0/387.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 170.0/341.9;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 198.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 8.0546875f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.51530611515f ) {
                                            return 149.0/142.0;
                                        } else {
                                            return 159.0/90.0;
                                        }
                                    } else {
                                        return 263.0/58.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.51530611515f ) {
                                        return 114.0/194.0;
                                    } else {
                                        return 223.0/176.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                    return 155.0/365.9;
                                } else {
                                    return 34.0/331.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 6647.0f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.213151931763f ) {
                                if ( rdb0_last_touched_diff <= 6306.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1629589.5f ) {
                                        return 24.0/503.9;
                                    } else {
                                        return 6.0/461.9;
                                    }
                                } else {
                                    return 42.0/497.9;
                                }
                            } else {
                                return 80.0/493.9;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 25.5f ) {
                                return 91.0/284.0;
                            } else {
                                return 25.0/361.9;
                            }
                        }
                    } else {
                        return 108.0/591.9;
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.844519734383f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 3370.0f ) {
                        return 73.0/262.0;
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 18777.5f ) {
                            return 154.0/112.0;
                        } else {
                            return 102.0/301.9;
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_used_for_uip_creation <= 1.5f ) {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 304.0f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 1.49689769745f ) {
                                    return 217.0/38.0;
                                } else {
                                    return 181.0/66.0;
                                }
                            } else {
                                return 144.0/120.0;
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                return 317.0/34.0;
                            } else {
                                return 210.0/60.0;
                            }
                        }
                    } else {
                        return 213.0/218.0;
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 7.5f ) {
            if ( cl->stats.glue <= 8.5f ) {
                if ( cl->stats.size_rel <= 0.475571393967f ) {
                    if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.0499400272965f ) {
                            if ( cl->stats.size_rel <= 0.0913903564215f ) {
                                return 134.0/425.9;
                            } else {
                                return 185.0/230.0;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 9.5f ) {
                                if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                    return 150.0/371.9;
                                } else {
                                    if ( cl->stats.glue <= 5.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.149444445968f ) {
                                            return 109.0/172.0;
                                        } else {
                                            return 94.0/262.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                            return 163.0/248.0;
                                        } else {
                                            return 209.0/180.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.762215971947f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.522063612938f ) {
                                        return 276.0/236.0;
                                    } else {
                                        return 166.0/276.0;
                                    }
                                } else {
                                    return 180.0/116.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 9.5f ) {
                            if ( cl->stats.glue <= 5.5f ) {
                                if ( rdb0_last_touched_diff <= 52069.5f ) {
                                    return 98.0/240.0;
                                } else {
                                    return 126.0/200.0;
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 15.5f ) {
                                    return 133.0/164.0;
                                } else {
                                    return 142.0/144.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 21.5f ) {
                                if ( cl->stats.num_overlap_literals <= 24.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0992954224348f ) {
                                        return 182.0/110.0;
                                    } else {
                                        return 269.0/70.0;
                                    }
                                } else {
                                    return 189.0/152.0;
                                }
                            } else {
                                if ( cl->stats.glue <= 4.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 53825.5f ) {
                                        return 131.0/180.0;
                                    } else {
                                        return 164.0/110.0;
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.190239176154f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 76780.0f ) {
                                            return 191.0/184.0;
                                        } else {
                                            return 184.0/114.0;
                                        }
                                    } else {
                                        return 183.0/88.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 67978.5f ) {
                        if ( cl->stats.sum_uip1_used <= 3.5f ) {
                            if ( cl->stats.num_overlap_literals <= 22.5f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                    return 238.0/238.0;
                                } else {
                                    return 200.0/130.0;
                                }
                            } else {
                                if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                    return 233.0/154.0;
                                } else {
                                    return 236.0/68.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 8.5f ) {
                                return 143.0/301.9;
                            } else {
                                return 141.0/140.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.58532166481f ) {
                            if ( rdb0_last_touched_diff <= 120703.0f ) {
                                return 146.0/136.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 10.0f ) {
                                    return 220.0/44.0;
                                } else {
                                    return 273.0/146.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 27.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.191527783871f ) {
                                    return 231.0/152.0;
                                } else {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                        return 360.0/48.0;
                                    } else {
                                        return 376.0/106.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 378206.5f ) {
                                    return 289.0/24.0;
                                } else {
                                    return 232.0/64.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.884767651558f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 11.5f ) {
                        if ( cl->stats.glue_rel_long <= 0.762604773045f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 118.5f ) {
                                if ( cl->stats.size_rel <= 0.728943705559f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.58333337307f ) {
                                        return 143.0/104.0;
                                    } else {
                                        return 186.0/90.0;
                                    }
                                } else {
                                    return 279.0/62.0;
                                }
                            } else {
                                return 343.0/78.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.382661938667f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.24473489821f ) {
                                    return 190.0/46.0;
                                } else {
                                    return 191.0/66.0;
                                }
                            } else {
                                if ( cl->stats.glue <= 14.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 6.29166698456f ) {
                                        return 285.0/44.0;
                                    } else {
                                        return 193.0/16.0;
                                    }
                                } else {
                                    return 200.0/38.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.817007064819f ) {
                            if ( rdb0_last_touched_diff <= 65188.5f ) {
                                if ( cl->size() <= 30.5f ) {
                                    return 225.0/397.9;
                                } else {
                                    return 234.0/190.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 233400.0f ) {
                                    if ( cl->stats.glue_rel_long <= 0.566717267036f ) {
                                        return 195.0/192.0;
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 99.0f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 7549.0f ) {
                                                return 139.0/112.0;
                                            } else {
                                                return 171.0/98.0;
                                            }
                                        } else {
                                            return 243.0/92.0;
                                        }
                                    }
                                } else {
                                    return 359.0/80.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 10.5f ) {
                                return 269.0/142.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 111.5f ) {
                                    return 250.0/94.0;
                                } else {
                                    return 195.0/34.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 2.5f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.672269999981f ) {
                            if ( cl->stats.size_rel <= 1.20460343361f ) {
                                if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 194.0f ) {
                                            if ( cl->stats.num_total_lits_antecedents <= 47.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.212458640337f ) {
                                                    return 190.0/52.0;
                                                } else {
                                                    return 162.0/72.0;
                                                }
                                            } else {
                                                if ( cl->stats.rdb1_last_touched_diff <= 32080.0f ) {
                                                    return 287.0/84.0;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 1.02202212811f ) {
                                                        if ( rdb0_last_touched_diff <= 97880.5f ) {
                                                            return 181.0/18.0;
                                                        } else {
                                                            return 212.0/50.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 77737.0f ) {
                                                            return 379.0/54.0;
                                                        } else {
                                                            if ( cl->stats.num_antecedents_rel <= 0.280935823917f ) {
                                                                return 190.0/12.0;
                                                            } else {
                                                                return 302.0/6.0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 183.0/102.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.407390058041f ) {
                                            return 364.0/108.0;
                                        } else {
                                            return 161.0/102.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                        return 249.0/46.0;
                                    } else {
                                        return 333.0/32.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 1.5f ) {
                                    if ( cl->stats.glue <= 33.5f ) {
                                        if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                            if ( cl->stats.dump_number <= 9.5f ) {
                                                return 213.0/38.0;
                                            } else {
                                                return 221.0/16.0;
                                            }
                                        } else {
                                            return 194.0/8.0;
                                        }
                                    } else {
                                        return 227.0/6.0;
                                    }
                                } else {
                                    return 188.0/38.0;
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 12.1737499237f ) {
                                if ( rdb0_last_touched_diff <= 422629.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 21.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 331.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 10.1494445801f ) {
                                                if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 47328.0f ) {
                                                        return 274.0/44.0;
                                                    } else {
                                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                            return 221.0/8.0;
                                                        } else {
                                                            return 275.0/22.0;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                                        return 343.0/94.0;
                                                    } else {
                                                        return 251.0/24.0;
                                                    }
                                                }
                                            } else {
                                                return 198.0/60.0;
                                            }
                                        } else {
                                            if ( cl->stats.size_rel <= 0.613944292068f ) {
                                                return 199.0/26.0;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 92832.5f ) {
                                                    return 237.0/18.0;
                                                } else {
                                                    return 1;
                                                }
                                            }
                                        }
                                    } else {
                                        return 370.0/78.0;
                                    }
                                } else {
                                    return 223.0/80.0;
                                }
                            } else {
                                if ( cl->stats.dump_number <= 35.5f ) {
                                    if ( rdb0_last_touched_diff <= 153287.0f ) {
                                        if ( cl->stats.num_antecedents_rel <= 2.36544084549f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                                if ( cl->stats.size_rel <= 0.927776694298f ) {
                                                    return 301.0/2.0;
                                                } else {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 49390.0f ) {
                                                        return 272.0/34.0;
                                                    } else {
                                                        if ( cl->stats.glue_rel_long <= 1.22876024246f ) {
                                                            return 199.0/22.0;
                                                        } else {
                                                            return 225.0/8.0;
                                                        }
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 1.064463377f ) {
                                                    if ( cl->stats.rdb1_last_touched_diff <= 52620.5f ) {
                                                        return 208.0/30.0;
                                                    } else {
                                                        return 218.0/54.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_overlap_literals <= 333.0f ) {
                                                        if ( cl->stats.glue_rel_long <= 1.28876543045f ) {
                                                            return 348.0/6.0;
                                                        } else {
                                                            return 374.0/40.0;
                                                        }
                                                    } else {
                                                        return 384.0/66.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 3.7435233593f ) {
                                                return 1;
                                            } else {
                                                return 200.0/22.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 1.08895981312f ) {
                                            return 347.0/30.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 1.54246914387f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 1.81039845943f ) {
                                                    if ( cl->stats.num_antecedents_rel <= 0.729653596878f ) {
                                                        return 196.0/6.0;
                                                    } else {
                                                        return 247.0/6.0;
                                                    }
                                                } else {
                                                    return 1;
                                                }
                                            } else {
                                                return 194.0/14.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 368.0/74.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 0.951575517654f ) {
                            return 312.0/176.0;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 195.5f ) {
                                if ( cl->stats.glue <= 10.5f ) {
                                    return 180.0/134.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 69904.0f ) {
                                        return 265.0/144.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 6.40277767181f ) {
                                            return 288.0/114.0;
                                        } else {
                                            return 284.0/40.0;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 106918.5f ) {
                                    if ( cl->stats.dump_number <= 6.5f ) {
                                        return 220.0/36.0;
                                    } else {
                                        return 178.0/52.0;
                                    }
                                } else {
                                    return 324.0/40.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                if ( cl->stats.size_rel <= 0.582543969154f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 58403.0f ) {
                        if ( cl->stats.sum_uip1_used <= 56.5f ) {
                            if ( cl->stats.sum_uip1_used <= 17.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0276650637388f ) {
                                    return 127.0/214.0;
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        return 105.0/363.9;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.382345020771f ) {
                                            return 203.0/292.0;
                                        } else {
                                            return 123.0/303.9;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 5729016.0f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 331288.5f ) {
                                        return 67.0/357.9;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 35425.5f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.0501579791307f ) {
                                                return 126.0/325.9;
                                            } else {
                                                return 126.0/491.9;
                                            }
                                        } else {
                                            return 106.0/423.9;
                                        }
                                    }
                                } else {
                                    return 128.0/234.0;
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 21.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0668560862541f ) {
                                    return 73.0/541.9;
                                } else {
                                    return 24.0/399.9;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.075626283884f ) {
                                    if ( rdb0_last_touched_diff <= 46464.0f ) {
                                        if ( cl->stats.sum_uip1_used <= 201.5f ) {
                                            return 86.0/347.9;
                                        } else {
                                            return 22.0/379.9;
                                        }
                                    } else {
                                        return 40.0/419.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 146.0f ) {
                                        return 98.0/270.0;
                                    } else {
                                        return 36.0/343.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 4950856.5f ) {
                            if ( cl->stats.sum_uip1_used <= 22.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.562947630882f ) {
                                    return 167.0/244.0;
                                } else {
                                    return 162.0/108.0;
                                }
                            } else {
                                return 168.0/451.9;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 49.5f ) {
                                return 65.0/274.0;
                            } else {
                                return 177.0/401.9;
                            }
                        }
                    }
                } else {
                    if ( cl->size() <= 41.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 6798165.5f ) {
                            if ( cl->stats.glue_rel_long <= 0.547554254532f ) {
                                return 135.0/194.0;
                            } else {
                                if ( cl->stats.dump_number <= 27.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 40216.0f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 52.5f ) {
                                            return 79.0/317.9;
                                        } else {
                                            return 86.0/220.0;
                                        }
                                    } else {
                                        return 166.0/266.0;
                                    }
                                } else {
                                    return 148.0/146.0;
                                }
                            }
                        } else {
                            return 111.0/437.9;
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.71522796154f ) {
                            return 156.0/144.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.205665946007f ) {
                                return 128.0/166.0;
                            } else {
                                return 136.0/226.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_queue <= 0.620251178741f ) {
                    if ( cl->stats.sum_uip1_used <= 54.5f ) {
                        if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                            if ( cl->stats.size_rel <= 0.296762615442f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.122197069228f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 22.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0669583454728f ) {
                                            return 173.0/286.0;
                                        } else {
                                            return 88.0/232.0;
                                        }
                                    } else {
                                        return 130.0/140.0;
                                    }
                                } else {
                                    return 84.0/363.9;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 110348.5f ) {
                                    return 178.0/449.9;
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.0903621539474f ) {
                                        return 170.0/200.0;
                                    } else {
                                        return 211.0/178.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 184051.5f ) {
                                return 148.0/210.0;
                            } else {
                                return 221.0/122.0;
                            }
                        }
                    } else {
                        if ( cl->stats.dump_number <= 17.5f ) {
                            return 33.0/473.9;
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 27.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.338097214699f ) {
                                    return 153.0/313.9;
                                } else {
                                    return 121.0/547.9;
                                }
                            } else {
                                return 223.0/349.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 625734.0f ) {
                        if ( rdb0_last_touched_diff <= 96770.0f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 64.5f ) {
                                return 115.0/198.0;
                            } else {
                                return 157.0/112.0;
                            }
                        } else {
                            if ( cl->stats.num_total_lits_antecedents <= 45.5f ) {
                                return 199.0/238.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.982174634933f ) {
                                    return 334.0/138.0;
                                } else {
                                    return 180.0/26.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 32.5f ) {
                            if ( cl->size() <= 12.5f ) {
                                return 132.0/210.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.921234667301f ) {
                                    return 263.0/114.0;
                                } else {
                                    return 142.0/156.0;
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.12990680337f ) {
                                return 120.0/206.0;
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.299204647541f ) {
                                    return 91.0/343.9;
                                } else {
                                    return 111.0/292.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_8(
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
    if ( cl->stats.antecedents_glue_long_reds_var <= 2.40117192268f ) {
        if ( rdb0_act_ranking_top_10 <= 2.5f ) {
            if ( rdb0_last_touched_diff <= 10922.0f ) {
                if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                    if ( cl->stats.sum_uip1_used <= 28.5f ) {
                        if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                            if ( cl->stats.num_overlap_literals <= 45.5f ) {
                                if ( rdb0_last_touched_diff <= 6773.5f ) {
                                    if ( cl->stats.size_rel <= 0.567925810814f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.647249937057f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 2711.5f ) {
                                                return 43.0/571.9;
                                            } else {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0843295678496f ) {
                                                    return 59.0/321.9;
                                                } else {
                                                    return 53.0/465.9;
                                                }
                                            }
                                        } else {
                                            return 100.0/407.9;
                                        }
                                    } else {
                                        return 77.0/268.0;
                                    }
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.245000004768f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.563169717789f ) {
                                            return 128.0/489.9;
                                        } else {
                                            return 67.0/369.9;
                                        }
                                    } else {
                                        return 115.0/339.9;
                                    }
                                }
                            } else {
                                return 119.0/250.0;
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.76222205162f ) {
                                if ( cl->size() <= 10.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 151639.0f ) {
                                        return 74.0/449.9;
                                    } else {
                                        if ( cl->stats.dump_number <= 13.5f ) {
                                            return 84.0/331.9;
                                        } else {
                                            return 118.0/200.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.176696524024f ) {
                                        return 110.0/286.0;
                                    } else {
                                        return 132.0/244.0;
                                    }
                                }
                            } else {
                                return 219.0/331.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                            if ( cl->stats.dump_number <= 28.5f ) {
                                if ( cl->stats.sum_uip1_used <= 43.5f ) {
                                    if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                        return 37.0/323.9;
                                    } else {
                                        return 63.0/303.9;
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 8883.0f ) {
                                            return 18.0/361.9;
                                        } else {
                                            return 45.0/371.9;
                                        }
                                    } else {
                                        if ( cl->stats.dump_number <= 14.5f ) {
                                            return 8.0/419.9;
                                        } else {
                                            return 43.0/559.9;
                                        }
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 7044.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 283.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.157937422395f ) {
                                            if ( cl->stats.glue <= 4.5f ) {
                                                return 62.0/321.9;
                                            } else {
                                                return 78.0/288.0;
                                            }
                                        } else {
                                            return 54.0/495.9;
                                        }
                                    } else {
                                        return 14.0/397.9;
                                    }
                                } else {
                                    return 122.0/547.9;
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 19.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 33833620.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1324765.5f ) {
                                            return 32.0/337.9;
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 2561.0f ) {
                                                if ( cl->stats.dump_number <= 17.5f ) {
                                                    return 7.0/597.9;
                                                } else {
                                                    return 35.0/521.9;
                                                }
                                            } else {
                                                return 51.0/687.9;
                                            }
                                        }
                                    } else {
                                        return 45.0/389.9;
                                    }
                                } else {
                                    if ( cl->size() <= 6.5f ) {
                                        return 12.0/601.9;
                                    } else {
                                        return 21.0/465.9;
                                    }
                                }
                            } else {
                                return 68.0/609.9;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 6192.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.0739792436361f ) {
                            if ( rdb0_last_touched_diff <= 5962.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 3080736.5f ) {
                                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                        if ( cl->stats.size_rel <= 0.367969572544f ) {
                                            if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                                return 24.0/643.9;
                                            } else {
                                                return 51.0/647.9;
                                            }
                                        } else {
                                            return 45.0/313.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 3483.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 206257.5f ) {
                                                return 22.0/489.9;
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.206976220012f ) {
                                                    return 21.0/371.9;
                                                } else {
                                                    if ( cl->stats.used_for_uip_creation <= 15.5f ) {
                                                        return 23.0/665.9;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 10.5f ) {
                                                            if ( cl->stats.size_rel <= 0.167340517044f ) {
                                                                return 14.0/393.9;
                                                            } else {
                                                                return 4.0/553.9;
                                                            }
                                                        } else {
                                                            return 1.0/419.9;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 31.0/401.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 5.5f ) {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.042043287307f ) {
                                            return 25.0/465.9;
                                        } else {
                                            return 8.0/429.9;
                                        }
                                    } else {
                                        if ( cl->stats.rdb1_used_for_uip_creation <= 5.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.0751264169812f ) {
                                                return 25.0/379.9;
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 12645831.0f ) {
                                                    return 16.0/377.9;
                                                } else {
                                                    return 8.0/693.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.0682197511196f ) {
                                                if ( rdb0_last_touched_diff <= 1118.5f ) {
                                                    if ( cl->stats.dump_number <= 8.5f ) {
                                                        return 0.0/553.9;
                                                    } else {
                                                        if ( cl->stats.rdb1_last_touched_diff <= 933.0f ) {
                                                            return 6.0/767.9;
                                                        } else {
                                                            return 16.0/373.9;
                                                        }
                                                    }
                                                } else {
                                                    if ( cl->stats.dump_number <= 15.5f ) {
                                                        return 23.0/381.9;
                                                    } else {
                                                        return 17.0/467.9;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_long <= 0.677170872688f ) {
                                                    if ( cl->stats.sum_delta_confl_uip1_used <= 309347072.0f ) {
                                                        if ( cl->stats.sum_uip1_used <= 108.5f ) {
                                                            return 18.0/357.9;
                                                        } else {
                                                            if ( cl->stats.size_rel <= 0.468210011721f ) {
                                                                if ( cl->stats.num_antecedents_rel <= 0.149660468102f ) {
                                                                    if ( cl->stats.used_for_uip_creation <= 27.5f ) {
                                                                        return 0.0/821.9;
                                                                    } else {
                                                                        return 5.0/729.9;
                                                                    }
                                                                } else {
                                                                    if ( cl->stats.num_antecedents_rel <= 0.174421563745f ) {
                                                                        return 10.0/387.9;
                                                                    } else {
                                                                        if ( cl->stats.used_for_uip_creation <= 38.5f ) {
                                                                            return 1.0/643.9;
                                                                        } else {
                                                                            return 5.0/407.9;
                                                                        }
                                                                    }
                                                                }
                                                            } else {
                                                                return 7.0/407.9;
                                                            }
                                                        }
                                                    } else {
                                                        return 16.0/459.9;
                                                    }
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 931.0f ) {
                                                        return 0.0/545.9;
                                                    } else {
                                                        return 2.0/415.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                    return 50.0/339.9;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                        return 7.0/479.9;
                                    } else {
                                        return 41.0/439.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.573049783707f ) {
                                if ( cl->stats.size_rel <= 0.242753535509f ) {
                                    if ( cl->stats.sum_uip1_used <= 79.5f ) {
                                        return 41.0/475.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0557088851929f ) {
                                            return 16.0/575.9;
                                        } else {
                                            return 23.0/377.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.455659627914f ) {
                                        if ( rdb0_last_touched_diff <= 1878.5f ) {
                                            return 20.0/541.9;
                                        } else {
                                            return 23.0/349.9;
                                        }
                                    } else {
                                        return 13.0/575.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.dump_number <= 3.5f ) {
                                    return 66.0/527.9;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.835916280746f ) {
                                        if ( cl->stats.sum_uip1_used <= 105.0f ) {
                                            return 26.0/369.9;
                                        } else {
                                            return 7.0/423.9;
                                        }
                                    } else {
                                        return 34.0/447.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 25.5f ) {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.176384136081f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 270608.5f ) {
                                    return 38.0/351.9;
                                } else {
                                    return 101.0/541.9;
                                }
                            } else {
                                return 118.0/431.9;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0648943036795f ) {
                                if ( cl->stats.sum_uip1_used <= 338.5f ) {
                                    if ( rdb0_last_touched_diff <= 1549.0f ) {
                                        return 40.0/489.9;
                                    } else {
                                        return 82.0/541.9;
                                    }
                                } else {
                                    return 18.0/423.9;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 31.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 143.5f ) {
                                        if ( cl->stats.dump_number <= 23.5f ) {
                                            if ( cl->stats.glue_rel_long <= 0.516446590424f ) {
                                                return 18.0/619.9;
                                            } else {
                                                return 22.0/389.9;
                                            }
                                        } else {
                                            return 26.0/357.9;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 1589.5f ) {
                                            return 2.0/391.9;
                                        } else {
                                            return 20.0/495.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 103.5f ) {
                                        if ( rdb0_last_touched_diff <= 1876.5f ) {
                                            return 28.0/349.9;
                                        } else {
                                            return 66.0/311.9;
                                        }
                                    } else {
                                        return 38.0/717.9;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 2189240.0f ) {
                    if ( cl->size() <= 11.5f ) {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.dump_number <= 18.5f ) {
                                if ( cl->stats.sum_uip1_used <= 19.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0324765555561f ) {
                                            return 39.0/355.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.100363455713f ) {
                                                return 103.0/371.9;
                                            } else {
                                                return 69.0/401.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->size() <= 4.5f ) {
                                            return 75.0/337.9;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.0873528420925f ) {
                                                return 98.0/214.0;
                                            } else {
                                                return 106.0/359.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 0.373613476753f ) {
                                        return 77.0/403.9;
                                    } else {
                                        return 36.0/465.9;
                                    }
                                }
                            } else {
                                return 196.0/280.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.209011077881f ) {
                                    if ( cl->stats.sum_uip1_used <= 21.5f ) {
                                        if ( cl->stats.glue_rel_queue <= 0.618930220604f ) {
                                            if ( cl->stats.glue <= 5.5f ) {
                                                return 130.0/232.0;
                                            } else {
                                                return 135.0/345.9;
                                            }
                                        } else {
                                            return 112.0/431.9;
                                        }
                                    } else {
                                        return 44.0/431.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 19271.0f ) {
                                        return 89.0/260.0;
                                    } else {
                                        return 125.0/230.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 24.5f ) {
                                    return 142.0/254.0;
                                } else {
                                    return 160.0/190.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 170.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.774661779404f ) {
                                    return 227.0/284.0;
                                } else {
                                    return 300.0/138.0;
                                }
                            } else {
                                return 269.0/44.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 31505.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.775133609772f ) {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                            if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 115133.0f ) {
                                                    return 102.0/234.0;
                                                } else {
                                                    return 148.0/154.0;
                                                }
                                            } else {
                                                return 92.0/236.0;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 12450.5f ) {
                                                return 72.0/286.0;
                                            } else {
                                                return 99.0/270.0;
                                            }
                                        }
                                    } else {
                                        return 82.0/274.0;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 17015.5f ) {
                                        return 116.0/224.0;
                                    } else {
                                        return 247.0/262.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    return 206.0/345.9;
                                } else {
                                    if ( cl->size() <= 17.5f ) {
                                        return 125.0/160.0;
                                    } else {
                                        return 183.0/110.0;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 49293.0f ) {
                        if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                            if ( cl->stats.sum_uip1_used <= 96.5f ) {
                                if ( rdb0_last_touched_diff <= 19430.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 57.5f ) {
                                        return 108.0/409.9;
                                    } else {
                                        return 67.0/419.9;
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 45.5f ) {
                                        return 158.0/381.9;
                                    } else {
                                        return 119.0/477.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.268036961555f ) {
                                    return 87.0/681.9;
                                } else {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0461797490716f ) {
                                        return 40.0/341.9;
                                    } else {
                                        if ( cl->size() <= 7.5f ) {
                                            if ( cl->stats.size_rel <= 0.206730097532f ) {
                                                return 12.0/447.9;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 19107.0f ) {
                                                    return 31.0/339.9;
                                                } else {
                                                    return 17.0/387.9;
                                                }
                                            }
                                        } else {
                                            return 38.0/507.9;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 88.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.0826266855001f ) {
                                    return 133.0/238.0;
                                } else {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.653333306313f ) {
                                        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                                            return 66.0/299.9;
                                        } else {
                                            return 73.0/278.0;
                                        }
                                    } else {
                                        return 91.0/236.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.size_rel <= 0.218263804913f ) {
                                    return 57.0/333.9;
                                } else {
                                    if ( rdb0_last_touched_diff <= 23769.5f ) {
                                        return 60.0/669.9;
                                    } else {
                                        return 62.0/437.9;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                            return 91.0/250.0;
                        } else {
                            return 163.0/274.0;
                        }
                    }
                }
            }
        } else {
            if ( cl->stats.sum_delta_confl_uip1_used <= 499.5f ) {
                if ( cl->stats.size_rel <= 0.603045225143f ) {
                    if ( cl->size() <= 6.5f ) {
                        if ( rdb0_last_touched_diff <= 68004.5f ) {
                            return 141.0/419.9;
                        } else {
                            if ( cl->stats.glue_rel_queue <= 0.43605017662f ) {
                                return 164.0/174.0;
                            } else {
                                return 140.0/198.0;
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 97997.0f ) {
                            if ( rdb0_last_touched_diff <= 49201.0f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.13191652298f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 3.5f ) {
                                        return 142.0/254.0;
                                    } else {
                                        return 166.0/216.0;
                                    }
                                } else {
                                    return 254.0/206.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 62071.5f ) {
                                    return 178.0/98.0;
                                } else {
                                    if ( cl->stats.glue <= 7.5f ) {
                                        return 160.0/162.0;
                                    } else {
                                        return 158.0/92.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 45.5f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    return 170.0/122.0;
                                } else {
                                    if ( cl->stats.glue_rel_long <= 0.675886631012f ) {
                                        if ( cl->size() <= 12.5f ) {
                                            return 303.0/134.0;
                                        } else {
                                            return 156.0/96.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 0.863133251667f ) {
                                            return 183.0/56.0;
                                        } else {
                                            return 221.0/36.0;
                                        }
                                    }
                                }
                            } else {
                                return 265.0/64.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.dump_number <= 7.5f ) {
                        if ( rdb0_last_touched_diff <= 27525.5f ) {
                            return 246.0/214.0;
                        } else {
                            if ( cl->size() <= 51.5f ) {
                                if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                    return 186.0/110.0;
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 79.5f ) {
                                        return 210.0/38.0;
                                    } else {
                                        return 154.0/90.0;
                                    }
                                }
                            } else {
                                return 227.0/52.0;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.748128652573f ) {
                            if ( cl->stats.dump_number <= 16.5f ) {
                                return 199.0/78.0;
                            } else {
                                return 242.0/70.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 0.0902901142836f ) {
                                return 200.0/82.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.988561749458f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.552433311939f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1.5f ) {
                                            return 226.0/14.0;
                                        } else {
                                            return 244.0/40.0;
                                        }
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.805377244949f ) {
                                            return 193.0/74.0;
                                        } else {
                                            return 214.0/42.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 175.5f ) {
                                        if ( rdb0_last_touched_diff <= 248179.5f ) {
                                            return 318.0/44.0;
                                        } else {
                                            return 205.0/10.0;
                                        }
                                    } else {
                                        return 210.0/6.0;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if ( cl->size() <= 9.5f ) {
                    if ( cl->stats.rdb1_last_touched_diff <= 76239.5f ) {
                        if ( cl->stats.sum_uip1_used <= 12.5f ) {
                            if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                return 103.0/309.9;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 65993.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.0836123302579f ) {
                                        return 76.0/278.0;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.239653289318f ) {
                                            return 102.0/202.0;
                                        } else {
                                            return 96.0/250.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                        return 128.0/200.0;
                                    } else {
                                        return 142.0/194.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 7191.0f ) {
                                return 70.0/649.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 68.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 1361478.0f ) {
                                        if ( cl->stats.dump_number <= 8.5f ) {
                                            return 91.0/539.9;
                                        } else {
                                            return 101.0/365.9;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_long <= 0.38886910677f ) {
                                            return 118.0/176.0;
                                        } else {
                                            return 108.0/292.0;
                                        }
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 60438.5f ) {
                                        return 61.0/629.9;
                                    } else {
                                        return 53.0/292.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 176514.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.169105097651f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 12645592.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 38.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 14.5f ) {
                                            return 212.0/367.9;
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 5.5f ) {
                                                return 162.0/162.0;
                                            } else {
                                                return 132.0/186.0;
                                            }
                                        }
                                    } else {
                                        return 108.0/319.9;
                                    }
                                } else {
                                    return 57.0/290.0;
                                }
                            } else {
                                return 122.0/160.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.0496016852558f ) {
                                return 251.0/206.0;
                            } else {
                                if ( cl->stats.dump_number <= 72.5f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                            return 190.0/284.0;
                                        } else {
                                            return 149.0/152.0;
                                        }
                                    } else {
                                        return 179.0/162.0;
                                    }
                                } else {
                                    return 107.0/198.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 77766.5f ) {
                        if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 24659.5f ) {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.0210453364998f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1727335.0f ) {
                                            if ( rdb0_last_touched_diff <= 22908.5f ) {
                                                return 142.0/301.9;
                                            } else {
                                                return 130.0/505.9;
                                            }
                                        } else {
                                            return 47.0/311.9;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 36.5f ) {
                                            return 148.0/292.0;
                                        } else {
                                            return 157.0/120.0;
                                        }
                                    }
                                } else {
                                    return 78.0/493.9;
                                }
                            } else {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.0802560448647f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 129.0/401.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.180330485106f ) {
                                            return 231.0/264.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.677714467049f ) {
                                                return 134.0/411.9;
                                            } else {
                                                return 140.0/196.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.glue <= 9.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 7.5f ) {
                                            return 211.0/152.0;
                                        } else {
                                            return 173.0/399.9;
                                        }
                                    } else {
                                        return 230.0/154.0;
                                    }
                                }
                            }
                        } else {
                            return 92.0/393.9;
                        }
                    } else {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 4430970.5f ) {
                            if ( cl->stats.dump_number <= 14.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.655914008617f ) {
                                    return 123.0/204.0;
                                } else {
                                    return 162.0/110.0;
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.611178994179f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 135615.0f ) {
                                        return 153.0/164.0;
                                    } else {
                                        if ( cl->stats.rdb1_last_touched_diff <= 328170.5f ) {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                return 284.0/168.0;
                                            } else {
                                                return 185.0/144.0;
                                            }
                                        } else {
                                            return 248.0/106.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 18.5f ) {
                                        if ( cl->stats.size_rel <= 0.956437349319f ) {
                                            if ( cl->stats.size_rel <= 0.811529695988f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.236467152834f ) {
                                                    if ( cl->stats.glue_rel_queue <= 0.793454051018f ) {
                                                        return 207.0/38.0;
                                                    } else {
                                                        return 187.0/66.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.antec_num_total_lits_rel <= 0.340364068747f ) {
                                                        return 164.0/106.0;
                                                    } else {
                                                        return 219.0/70.0;
                                                    }
                                                }
                                            } else {
                                                return 152.0/94.0;
                                            }
                                        } else {
                                            return 332.0/70.0;
                                        }
                                    } else {
                                        return 193.0/176.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.dump_number <= 54.5f ) {
                                return 112.0/234.0;
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                    return 174.0/252.0;
                                } else {
                                    return 178.0/138.0;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if ( cl->stats.sum_uip1_used <= 6.5f ) {
            if ( cl->stats.size_rel <= 0.733617722988f ) {
                if ( cl->stats.antecedents_glue_long_reds_var <= 12.5668926239f ) {
                    if ( cl->stats.glue_rel_long <= 0.737055420876f ) {
                        if ( cl->stats.dump_number <= 11.5f ) {
                            if ( rdb0_last_touched_diff <= 14124.5f ) {
                                return 79.0/258.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 77.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 792.0f ) {
                                        return 220.0/240.0;
                                    } else {
                                        return 124.0/204.0;
                                    }
                                } else {
                                    return 233.0/158.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                return 235.0/96.0;
                            } else {
                                return 182.0/128.0;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 22242.0f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 249.0/329.9;
                            } else {
                                return 302.0/166.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals <= 33.5f ) {
                                return 318.0/190.0;
                            } else {
                                if ( cl->stats.num_overlap_literals <= 366.5f ) {
                                    if ( cl->stats.antec_num_total_lits_rel <= 0.392453074455f ) {
                                        return 339.0/70.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.730258822441f ) {
                                            return 231.0/122.0;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 16.5f ) {
                                                return 389.0/52.0;
                                            } else {
                                                return 162.0/104.0;
                                            }
                                        }
                                    }
                                } else {
                                    return 249.0/28.0;
                                }
                            }
                        }
                    }
                } else {
                    if ( rdb0_act_ranking_top_10 <= 1.5f ) {
                        return 271.0/168.0;
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.810712218285f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                return 168.0/88.0;
                            } else {
                                return 154.0/114.0;
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 51.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 414.0f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 217010.5f ) {
                                            if ( cl->stats.num_antecedents_rel <= 0.1811003685f ) {
                                                return 170.0/38.0;
                                            } else {
                                                if ( rdb0_last_touched_diff <= 32988.0f ) {
                                                    return 200.0/34.0;
                                                } else {
                                                    if ( rdb0_act_ranking_top_10 <= 5.5f ) {
                                                        return 375.0/14.0;
                                                    } else {
                                                        return 269.0/30.0;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 197.0/50.0;
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.08593797684f ) {
                                            return 213.0/4.0;
                                        } else {
                                            return 272.0/12.0;
                                        }
                                    }
                                } else {
                                    return 195.0/50.0;
                                }
                            } else {
                                return 251.0/82.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.glue_rel_long <= 1.00024819374f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 23.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 11131.0f ) {
                            return 169.0/76.0;
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 1.90287935734f ) {
                                if ( cl->stats.num_overlap_literals <= 126.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                        if ( cl->size() <= 34.5f ) {
                                            if ( cl->stats.size_rel <= 1.07742071152f ) {
                                                return 350.0/54.0;
                                            } else {
                                                return 273.0/64.0;
                                            }
                                        } else {
                                            return 315.0/26.0;
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 116180.0f ) {
                                            return 207.0/72.0;
                                        } else {
                                            return 215.0/42.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 9.5f ) {
                                        return 231.0/8.0;
                                    } else {
                                        return 230.0/32.0;
                                    }
                                }
                            } else {
                                return 194.0/72.0;
                            }
                        }
                    } else {
                        if ( cl->stats.glue_rel_long <= 0.692448437214f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 63096.0f ) {
                                return 128.0/160.0;
                            } else {
                                return 174.0/92.0;
                            }
                        } else {
                            if ( cl->stats.num_overlap_literals_rel <= 1.20771241188f ) {
                                if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                    if ( cl->stats.glue_rel_queue <= 0.81951880455f ) {
                                        return 208.0/84.0;
                                    } else {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 9.07024765015f ) {
                                            return 145.0/102.0;
                                        } else {
                                            return 187.0/86.0;
                                        }
                                    }
                                } else {
                                    if ( cl->size() <= 30.5f ) {
                                        return 182.0/42.0;
                                    } else {
                                        return 200.0/32.0;
                                    }
                                }
                            } else {
                                return 209.0/160.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 8.5f ) {
                        if ( cl->stats.sum_delta_confl_uip1_used <= 36444.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 6.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 41116.5f ) {
                                    if ( cl->stats.glue_rel_long <= 1.38977169991f ) {
                                        if ( cl->stats.glue <= 16.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                if ( cl->stats.num_overlap_literals_rel <= 0.934056639671f ) {
                                                    return 193.0/46.0;
                                                } else {
                                                    return 235.0/30.0;
                                                }
                                            } else {
                                                return 196.0/70.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 428.5f ) {
                                                if ( cl->size() <= 85.5f ) {
                                                    return 312.0/30.0;
                                                } else {
                                                    return 186.0/48.0;
                                                }
                                            } else {
                                                return 319.0/8.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 1.1384049654f ) {
                                            return 182.0/32.0;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                                return 354.0/6.0;
                                            } else {
                                                return 222.0/30.0;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 388748.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 115.5f ) {
                                            if ( rdb0_last_touched_diff <= 124855.5f ) {
                                                return 335.0/58.0;
                                            } else {
                                                return 246.0/10.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.991302669048f ) {
                                                if ( cl->stats.dump_number <= 13.5f ) {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.724491000175f ) {
                                                        return 321.0/4.0;
                                                    } else {
                                                        return 402.0/12.0;
                                                    }
                                                } else {
                                                    return 375.0/2.0;
                                                }
                                            } else {
                                                if ( cl->stats.glue_rel_queue <= 1.1692109108f ) {
                                                    if ( cl->stats.glue_rel_long <= 1.12837600708f ) {
                                                        return 241.0/2.0;
                                                    } else {
                                                        return 201.0/8.0;
                                                    }
                                                } else {
                                                    if ( cl->size() <= 62.5f ) {
                                                        return 408.0/42.0;
                                                    } else {
                                                        return 222.0/4.0;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        return 204.0/26.0;
                                    }
                                }
                            } else {
                                if ( cl->size() <= 24.5f ) {
                                    return 200.0/110.0;
                                } else {
                                    if ( cl->stats.size_rel <= 1.14541852474f ) {
                                        return 220.0/54.0;
                                    } else {
                                        if ( cl->size() <= 166.0f ) {
                                            if ( cl->stats.glue_rel_queue <= 1.18929743767f ) {
                                                return 219.0/26.0;
                                            } else {
                                                return 410.0/28.0;
                                            }
                                        } else {
                                            return 194.0/34.0;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                return 184.0/128.0;
                            } else {
                                return 189.0/60.0;
                            }
                        }
                    } else {
                        if ( cl->stats.size_rel <= 0.991282463074f ) {
                            return 242.0/28.0;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 128075.0f ) {
                                return 208.0/14.0;
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 9.5f ) {
                                    if ( rdb0_act_ranking_top_10 <= 9.5f ) {
                                        return 1;
                                    } else {
                                        return 244.0/2.0;
                                    }
                                } else {
                                    return 212.0/6.0;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 41076.0f ) {
                if ( rdb0_last_touched_diff <= 8282.5f ) {
                    if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                        if ( cl->stats.size_rel <= 0.702307343483f ) {
                            if ( cl->stats.num_total_lits_antecedents <= 154.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 1443.0f ) {
                                    return 39.0/665.9;
                                } else {
                                    if ( cl->stats.used_for_uip_creation <= 3.5f ) {
                                        return 60.0/303.9;
                                    } else {
                                        return 54.0/611.9;
                                    }
                                }
                            } else {
                                return 18.0/471.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 10.5f ) {
                                if ( cl->stats.num_overlap_literals_rel <= 0.423364818096f ) {
                                    return 75.0/345.9;
                                } else {
                                    return 78.0/244.0;
                                }
                            } else {
                                return 18.0/383.9;
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                            if ( rdb0_last_touched_diff <= 2890.5f ) {
                                return 69.0/403.9;
                            } else {
                                return 95.0/303.9;
                            }
                        } else {
                            return 49.0/347.9;
                        }
                    }
                } else {
                    if ( cl->stats.num_total_lits_antecedents <= 35.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 9146.0f ) {
                            return 44.0/331.9;
                        } else {
                            return 73.0/347.9;
                        }
                    } else {
                        if ( cl->stats.glue_rel_queue <= 1.05123639107f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 9.5f ) {
                                if ( cl->size() <= 20.5f ) {
                                    if ( cl->size() <= 13.5f ) {
                                        return 75.0/341.9;
                                    } else {
                                        return 126.0/395.9;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.45066729188f ) {
                                        if ( cl->stats.size_rel <= 0.652288675308f ) {
                                            return 91.0/226.0;
                                        } else {
                                            return 211.0/301.9;
                                        }
                                    } else {
                                        return 82.0/278.0;
                                    }
                                }
                            } else {
                                return 57.0/284.0;
                            }
                        } else {
                            return 167.0/315.9;
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 383824.0f ) {
                    if ( cl->stats.glue_rel_queue <= 0.99124622345f ) {
                        if ( cl->stats.num_overlap_literals <= 72.5f ) {
                            if ( cl->stats.dump_number <= 11.5f ) {
                                return 132.0/208.0;
                            } else {
                                if ( cl->size() <= 20.5f ) {
                                    return 149.0/116.0;
                                } else {
                                    return 155.0/80.0;
                                }
                            }
                        } else {
                            return 225.0/90.0;
                        }
                    } else {
                        return 298.0/62.0;
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 6825329.5f ) {
                        if ( cl->stats.rdb1_last_touched_diff <= 154495.5f ) {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.744153738022f ) {
                                    return 125.0/206.0;
                                } else {
                                    return 200.0/160.0;
                                }
                            } else {
                                return 117.0/254.0;
                            }
                        } else {
                            return 224.0/144.0;
                        }
                    } else {
                        return 132.0/341.9;
                    }
                }
            }
        }
    }
}

static double estimator_should_keep_long_conf3_cluster0_9(
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
    if ( cl->stats.glue_rel_queue <= 0.803066253662f ) {
        if ( cl->stats.rdb1_last_touched_diff <= 45368.0f ) {
            if ( cl->stats.sum_uip1_used <= 22.5f ) {
                if ( cl->stats.size_rel <= 0.774073958397f ) {
                    if ( cl->stats.used_for_uip_creation <= 0.5f ) {
                        if ( cl->stats.num_overlap_literals <= 17.5f ) {
                            if ( cl->size() <= 8.5f ) {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 1341874.5f ) {
                                    if ( cl->stats.dump_number <= 12.5f ) {
                                        if ( cl->size() <= 4.5f ) {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 0.5f ) {
                                                return 111.0/597.9;
                                            } else {
                                                return 61.0/399.9;
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                                if ( cl->stats.antec_num_total_lits_rel <= 0.0401835627854f ) {
                                                    return 142.0/315.9;
                                                } else {
                                                    if ( cl->stats.dump_number <= 6.5f ) {
                                                        if ( cl->stats.size_rel <= 0.199191018939f ) {
                                                            return 136.0/307.9;
                                                        } else {
                                                            if ( cl->stats.num_overlap_literals_rel <= 0.0765365809202f ) {
                                                                return 68.0/391.9;
                                                            } else {
                                                                return 130.0/419.9;
                                                            }
                                                        }
                                                    } else {
                                                        return 78.0/457.9;
                                                    }
                                                }
                                            } else {
                                                return 45.0/345.9;
                                            }
                                        }
                                    } else {
                                        return 198.0/309.9;
                                    }
                                } else {
                                    return 125.0/200.0;
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 44974.0f ) {
                                    if ( cl->stats.num_total_lits_antecedents <= 30.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 0.0574833229184f ) {
                                            return 116.0/210.0;
                                        } else {
                                            if ( cl->stats.sum_uip1_used <= 3.5f ) {
                                                if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                                    return 131.0/248.0;
                                                } else {
                                                    return 96.0/220.0;
                                                }
                                            } else {
                                                if ( cl->size() <= 9.5f ) {
                                                    return 124.0/309.9;
                                                } else {
                                                    if ( cl->stats.glue_rel_queue <= 0.481618225574f ) {
                                                        return 74.0/337.9;
                                                    } else {
                                                        if ( cl->stats.glue_rel_queue <= 0.626141428947f ) {
                                                            return 125.0/317.9;
                                                        } else {
                                                            return 64.0/280.0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 1149.0f ) {
                                            return 154.0/166.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 0.520363628864f ) {
                                                return 198.0/385.9;
                                            } else {
                                                return 89.0/311.9;
                                            }
                                        }
                                    }
                                } else {
                                    return 210.0/264.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 58.0f ) {
                                if ( cl->stats.glue <= 8.5f ) {
                                    if ( cl->stats.dump_number <= 2.5f ) {
                                        return 171.0/248.0;
                                    } else {
                                        return 152.0/134.0;
                                    }
                                } else {
                                    return 225.0/100.0;
                                }
                            } else {
                                if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                    if ( cl->stats.size_rel <= 0.194721505046f ) {
                                        return 81.0/292.0;
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.339241147041f ) {
                                            if ( cl->stats.size_rel <= 0.421328932047f ) {
                                                return 162.0/296.0;
                                            } else {
                                                return 205.0/204.0;
                                            }
                                        } else {
                                            return 144.0/425.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals_rel <= 0.103814914823f ) {
                                        return 161.0/172.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.468308389187f ) {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                return 158.0/284.0;
                                            } else {
                                                return 143.0/212.0;
                                            }
                                        } else {
                                            return 165.0/208.0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->size() <= 7.5f ) {
                            if ( cl->stats.used_for_uip_creation <= 1.5f ) {
                                if ( cl->stats.sum_uip1_used <= 9.5f ) {
                                    return 97.0/423.9;
                                } else {
                                    return 52.0/363.9;
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.0295680947602f ) {
                                    return 55.0/301.9;
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 8.5f ) {
                                            return 58.0/393.9;
                                        } else {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 338901.5f ) {
                                                return 14.0/471.9;
                                            } else {
                                                return 47.0/349.9;
                                            }
                                        }
                                    } else {
                                        return 25.0/603.9;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 4.5f ) {
                                if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                    if ( cl->size() <= 26.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.0673329904675f ) {
                                            return 98.0/222.0;
                                        } else {
                                            if ( cl->stats.glue_rel_queue <= 0.590086936951f ) {
                                                if ( rdb0_last_touched_diff <= 3641.0f ) {
                                                    return 42.0/399.9;
                                                } else {
                                                    return 123.0/393.9;
                                                }
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 0.571666657925f ) {
                                                    return 83.0/355.9;
                                                } else {
                                                    return 96.0/192.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 94.0/210.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.333530038595f ) {
                                        return 58.0/325.9;
                                    } else {
                                        return 56.0/463.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.rdb1_used_for_uip_creation <= 7.5f ) {
                                    return 38.0/395.9;
                                } else {
                                    return 39.0/335.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 5029.0f ) {
                        if ( rdb0_last_touched_diff <= 10669.5f ) {
                            return 84.0/238.0;
                        } else {
                            return 127.0/178.0;
                        }
                    } else {
                        if ( cl->stats.sum_uip1_used <= 2.5f ) {
                            if ( cl->stats.num_antecedents_rel <= 0.398088783026f ) {
                                return 188.0/58.0;
                            } else {
                                return 186.0/116.0;
                            }
                        } else {
                            if ( cl->stats.antec_num_total_lits_rel <= 0.270497113466f ) {
                                return 115.0/222.0;
                            } else {
                                if ( rdb0_last_touched_diff <= 28309.0f ) {
                                    return 234.0/319.9;
                                } else {
                                    return 210.0/164.0;
                                }
                            }
                        }
                    }
                }
            } else {
                if ( rdb0_last_touched_diff <= 11200.5f ) {
                    if ( cl->stats.used_for_uip_creation <= 6.5f ) {
                        if ( cl->size() <= 12.5f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 2.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 21943.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 41.5f ) {
                                        if ( cl->stats.glue_rel_long <= 0.488047242165f ) {
                                            return 58.0/499.9;
                                        } else {
                                            return 57.0/329.9;
                                        }
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 239.5f ) {
                                            if ( cl->stats.dump_number <= 13.5f ) {
                                                return 16.0/593.9;
                                            } else {
                                                if ( cl->stats.size_rel <= 0.228663504124f ) {
                                                    if ( cl->stats.size_rel <= 0.149150907993f ) {
                                                        return 45.0/441.9;
                                                    } else {
                                                        return 51.0/294.0;
                                                    }
                                                } else {
                                                    if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                        return 20.0/333.9;
                                                    } else {
                                                        return 53.0/517.9;
                                                    }
                                                }
                                            }
                                        } else {
                                            return 29.0/747.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.227354645729f ) {
                                        return 63.0/294.0;
                                    } else {
                                        return 31.0/363.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals_rel <= 0.00797034427524f ) {
                                    return 37.0/329.9;
                                } else {
                                    if ( rdb0_last_touched_diff <= 429.0f ) {
                                        return 5.0/483.9;
                                    } else {
                                        if ( cl->stats.sum_uip1_used <= 101.5f ) {
                                            if ( cl->stats.used_for_uip_creation <= 4.5f ) {
                                                if ( cl->stats.glue <= 3.5f ) {
                                                    return 38.0/489.9;
                                                } else {
                                                    if ( cl->stats.size_rel <= 0.229594960809f ) {
                                                        return 44.0/605.9;
                                                    } else {
                                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                                            return 27.0/511.9;
                                                        } else {
                                                            return 13.0/417.9;
                                                        }
                                                    }
                                                }
                                            } else {
                                                return 49.0/463.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 3.5f ) {
                                                return 12.0/707.9;
                                            } else {
                                                if ( cl->stats.num_antecedents_rel <= 0.0791971459985f ) {
                                                    return 34.0/371.9;
                                                } else {
                                                    if ( cl->stats.dump_number <= 25.5f ) {
                                                        return 26.0/559.9;
                                                    } else {
                                                        return 12.0/779.9;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 31.5f ) {
                                return 91.0/415.9;
                            } else {
                                if ( cl->stats.dump_number <= 48.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 26413332.0f ) {
                                        if ( cl->size() <= 15.5f ) {
                                            return 50.0/347.9;
                                        } else {
                                            if ( cl->size() <= 52.5f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 2979615.0f ) {
                                                    return 23.0/569.9;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 3907.5f ) {
                                                        return 43.0/371.9;
                                                    } else {
                                                        return 26.0/435.9;
                                                    }
                                                }
                                            } else {
                                                return 41.0/337.9;
                                            }
                                        }
                                    } else {
                                        return 9.0/397.9;
                                    }
                                } else {
                                    return 67.0/383.9;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 5162.0f ) {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 8.5f ) {
                                if ( rdb0_last_touched_diff <= 5331.5f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 18647464.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                            if ( cl->stats.dump_number <= 6.5f ) {
                                                return 14.0/441.9;
                                            } else {
                                                return 7.0/471.9;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.179698854685f ) {
                                                if ( cl->stats.glue_rel_queue <= 0.428607463837f ) {
                                                    return 38.0/361.9;
                                                } else {
                                                    return 29.0/365.9;
                                                }
                                            } else {
                                                return 28.0/767.9;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.0317050516605f ) {
                                            return 16.0/427.9;
                                        } else {
                                            return 7.0/681.9;
                                        }
                                    }
                                } else {
                                    return 45.0/321.9;
                                }
                            } else {
                                if ( cl->stats.used_for_uip_creation <= 20.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 100.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 23.5f ) {
                                            if ( cl->stats.sum_uip1_used <= 61.5f ) {
                                                return 12.0/563.9;
                                            } else {
                                                return 20.0/383.9;
                                            }
                                        } else {
                                            return 54.0/633.9;
                                        }
                                    } else {
                                        if ( cl->stats.used_for_uip_creation <= 8.5f ) {
                                            return 6.0/591.9;
                                        } else {
                                            if ( rdb0_last_touched_diff <= 3387.0f ) {
                                                if ( cl->stats.glue_rel_long <= 0.298556596041f ) {
                                                    return 19.0/591.9;
                                                } else {
                                                    if ( cl->stats.num_overlap_literals_rel <= 0.0390710607171f ) {
                                                        return 9.0/451.9;
                                                    } else {
                                                        return 6.0/621.9;
                                                    }
                                                }
                                            } else {
                                                return 23.0/465.9;
                                            }
                                        }
                                    }
                                } else {
                                    if ( cl->stats.rdb1_used_for_uip_creation <= 40.5f ) {
                                        if ( cl->size() <= 6.5f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2239192.0f ) {
                                                return 8.0/427.9;
                                            } else {
                                                if ( cl->stats.glue <= 3.5f ) {
                                                    return 0.0/545.9;
                                                } else {
                                                    return 2.0/565.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.rdb1_last_touched_diff <= 938.0f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.205398842692f ) {
                                                    return 6.0/777.9;
                                                } else {
                                                    return 7.0/415.9;
                                                }
                                            } else {
                                                return 19.0/703.9;
                                            }
                                        }
                                    } else {
                                        if ( rdb0_last_touched_diff <= 690.5f ) {
                                            if ( cl->stats.glue_rel_queue <= 0.280320823193f ) {
                                                return 21.0/705.9;
                                            } else {
                                                if ( cl->stats.rdb1_used_for_uip_creation <= 104.5f ) {
                                                    return 3.0/805.9;
                                                } else {
                                                    return 9.0/511.9;
                                                }
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 8.5f ) {
                                                return 17.0/701.9;
                                            } else {
                                                return 30.0/471.9;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.num_antecedents_rel <= 0.110031113029f ) {
                                if ( cl->stats.used_for_uip_creation <= 10.5f ) {
                                    return 57.0/475.9;
                                } else {
                                    if ( cl->stats.dump_number <= 15.5f ) {
                                        return 41.0/635.9;
                                    } else {
                                        return 15.0/351.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.515818655491f ) {
                                    if ( cl->stats.num_overlap_literals <= 5.5f ) {
                                        return 5.0/517.9;
                                    } else {
                                        if ( cl->stats.num_antecedents_rel <= 0.237514823675f ) {
                                            return 31.0/503.9;
                                        } else {
                                            return 17.0/403.9;
                                        }
                                    }
                                } else {
                                    return 46.0/665.9;
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 91.5f ) {
                        if ( cl->stats.dump_number <= 27.5f ) {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 0.721938788891f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 26.5f ) {
                                    if ( rdb0_last_touched_diff <= 13910.5f ) {
                                        return 29.0/475.9;
                                    } else {
                                        if ( cl->stats.size_rel <= 0.302030146122f ) {
                                            if ( cl->stats.sum_delta_confl_uip1_used <= 2282584.0f ) {
                                                if ( cl->stats.num_total_lits_antecedents <= 15.5f ) {
                                                    return 68.0/409.9;
                                                } else {
                                                    return 72.0/276.0;
                                                }
                                            } else {
                                                return 51.0/413.9;
                                            }
                                        } else {
                                            return 66.0/615.9;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.dump_number <= 14.5f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.116598233581f ) {
                                            return 54.0/288.0;
                                        } else {
                                            return 62.0/531.9;
                                        }
                                    } else {
                                        return 95.0/248.0;
                                    }
                                }
                            } else {
                                if ( rdb0_last_touched_diff <= 31629.5f ) {
                                    if ( cl->size() <= 16.5f ) {
                                        return 53.0/449.9;
                                    } else {
                                        return 83.0/305.9;
                                    }
                                } else {
                                    return 98.0/260.0;
                                }
                            }
                        } else {
                            if ( rdb0_last_touched_diff <= 42658.5f ) {
                                if ( cl->size() <= 11.5f ) {
                                    if ( cl->stats.dump_number <= 55.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 12848.0f ) {
                                            return 82.0/224.0;
                                        } else {
                                            return 64.0/288.0;
                                        }
                                    } else {
                                        return 112.0/260.0;
                                    }
                                } else {
                                    return 153.0/301.9;
                                }
                            } else {
                                return 123.0/180.0;
                            }
                        }
                    } else {
                        if ( cl->stats.num_antecedents_rel <= 0.0512737929821f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 27952848.0f ) {
                                return 58.0/319.9;
                            } else {
                                return 53.0/363.9;
                            }
                        } else {
                            if ( cl->stats.dump_number <= 26.5f ) {
                                if ( cl->stats.glue_rel_queue <= 0.401702642441f ) {
                                    if ( cl->stats.size_rel <= 0.248846262693f ) {
                                        return 47.0/563.9;
                                    } else {
                                        return 23.0/417.9;
                                    }
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 14120.5f ) {
                                        return 17.0/623.9;
                                    } else {
                                        return 28.0/415.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 79073728.0f ) {
                                    if ( cl->stats.sum_uip1_used <= 171.5f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 14116.0f ) {
                                            return 67.0/437.9;
                                        } else {
                                            return 82.0/355.9;
                                        }
                                    } else {
                                        return 63.0/629.9;
                                    }
                                } else {
                                    if ( rdb0_last_touched_diff <= 27065.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 447.5f ) {
                                            return 26.0/351.9;
                                        } else {
                                            return 22.0/421.9;
                                        }
                                    } else {
                                        return 39.0/323.9;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                if ( rdb0_last_touched_diff <= 113146.0f ) {
                    if ( cl->stats.glue <= 8.5f ) {
                        if ( cl->stats.dump_number <= 10.5f ) {
                            if ( cl->stats.sum_uip1_used <= 6.5f ) {
                                if ( rdb0_last_touched_diff <= 81979.0f ) {
                                    if ( cl->size() <= 9.5f ) {
                                        if ( cl->stats.num_total_lits_antecedents <= 20.5f ) {
                                            return 110.0/234.0;
                                        } else {
                                            return 116.0/186.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                            return 156.0/136.0;
                                        } else {
                                            return 150.0/106.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.235132575035f ) {
                                        return 180.0/170.0;
                                    } else {
                                        return 192.0/114.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.47913736105f ) {
                                    return 111.0/301.9;
                                } else {
                                    return 81.0/280.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_uip1_used <= 12.5f ) {
                                if ( rdb0_last_touched_diff <= 68955.5f ) {
                                    return 140.0/198.0;
                                } else {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 291373.5f ) {
                                        return 252.0/264.0;
                                    } else {
                                        return 226.0/122.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.antec_num_total_lits_rel <= 0.175364106894f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.076753526926f ) {
                                        return 162.0/397.9;
                                    } else {
                                        if ( cl->stats.num_overlap_literals <= 7.5f ) {
                                            if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                                return 90.0/489.9;
                                            } else {
                                                return 72.0/290.0;
                                            }
                                        } else {
                                            return 110.0/355.9;
                                        }
                                    }
                                } else {
                                    if ( rdb0_act_ranking_top_10 <= 3.5f ) {
                                        return 175.0/309.9;
                                    } else {
                                        return 79.0/272.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.num_total_lits_antecedents <= 115.5f ) {
                            if ( cl->stats.rdb1_act_ranking_top_10 <= 1.5f ) {
                                return 130.0/276.0;
                            } else {
                                if ( cl->stats.glue_rel_long <= 0.609288573265f ) {
                                    if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                        return 174.0/150.0;
                                    } else {
                                        return 94.0/216.0;
                                    }
                                } else {
                                    if ( cl->stats.size_rel <= 0.714817881584f ) {
                                        return 186.0/190.0;
                                    } else {
                                        return 194.0/86.0;
                                    }
                                }
                            }
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 2.93055558205f ) {
                                return 159.0/90.0;
                            } else {
                                return 273.0/106.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 21.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.glue_rel_queue <= 0.200536191463f ) {
                                return 121.0/200.0;
                            } else {
                                if ( cl->stats.rdb1_last_touched_diff <= 219729.0f ) {
                                    if ( cl->stats.num_overlap_literals <= 18.5f ) {
                                        if ( cl->stats.rdb1_act_ranking_top_10 <= 4.5f ) {
                                            return 187.0/114.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.096652366221f ) {
                                                return 134.0/224.0;
                                            } else {
                                                return 264.0/230.0;
                                            }
                                        }
                                    } else {
                                        return 118.0/178.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.191309139132f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 51906.5f ) {
                                            return 169.0/102.0;
                                        } else {
                                            return 144.0/150.0;
                                        }
                                    } else {
                                        return 308.0/142.0;
                                    }
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 4.5f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 281588.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.863129556179f ) {
                                        return 180.0/170.0;
                                    } else {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.328141689301f ) {
                                            return 147.0/98.0;
                                        } else {
                                            return 175.0/48.0;
                                        }
                                    }
                                } else {
                                    return 192.0/38.0;
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    if ( cl->stats.rdb1_last_touched_diff <= 401435.5f ) {
                                        if ( cl->stats.size_rel <= 0.601372480392f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 0.127564102411f ) {
                                                return 240.0/76.0;
                                            } else {
                                                return 186.0/82.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_antecedents_rel <= 0.587941110134f ) {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                    return 252.0/18.0;
                                                } else {
                                                    return 206.0/52.0;
                                                }
                                            } else {
                                                return 220.0/78.0;
                                            }
                                        }
                                    } else {
                                        return 306.0/44.0;
                                    }
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.263179779053f ) {
                                        if ( cl->stats.rdb1_last_touched_diff <= 266039.0f ) {
                                            if ( cl->stats.glue_rel_long <= 0.62338244915f ) {
                                                return 277.0/76.0;
                                            } else {
                                                return 169.0/80.0;
                                            }
                                        } else {
                                            return 277.0/58.0;
                                        }
                                    } else {
                                        if ( cl->stats.num_overlap_literals_rel <= 0.309601426125f ) {
                                            return 258.0/202.0;
                                        } else {
                                            if ( cl->stats.antec_num_total_lits_rel <= 0.649403333664f ) {
                                                return 198.0/64.0;
                                            } else {
                                                return 163.0/70.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 223324.5f ) {
                            if ( cl->stats.size_rel <= 0.43659311533f ) {
                                if ( cl->stats.size_rel <= 0.226274058223f ) {
                                    if ( cl->stats.num_antecedents_rel <= 0.112648323178f ) {
                                        return 111.0/196.0;
                                    } else {
                                        return 112.0/180.0;
                                    }
                                } else {
                                    return 141.0/523.9;
                                }
                            } else {
                                if ( cl->stats.num_total_lits_antecedents <= 46.5f ) {
                                    return 135.0/142.0;
                                } else {
                                    return 129.0/170.0;
                                }
                            }
                        } else {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 18663602.0f ) {
                                if ( cl->stats.num_total_lits_antecedents <= 25.5f ) {
                                    return 150.0/174.0;
                                } else {
                                    return 254.0/198.0;
                                }
                            } else {
                                return 138.0/252.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->size() <= 8.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 2020206.5f ) {
                        return 91.0/268.0;
                    } else {
                        return 82.0/511.9;
                    }
                } else {
                    if ( cl->stats.glue_rel_queue <= 0.64409506321f ) {
                        if ( rdb0_last_touched_diff <= 3686.0f ) {
                            return 103.0/445.9;
                        } else {
                            return 141.0/216.0;
                        }
                    } else {
                        return 131.0/236.0;
                    }
                }
            }
        }
    } else {
        if ( rdb0_act_ranking_top_10 <= 1.5f ) {
            if ( cl->stats.num_overlap_literals_rel <= 0.43851980567f ) {
                if ( cl->stats.sum_uip1_used <= 7.5f ) {
                    if ( cl->size() <= 13.5f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.3671875f ) {
                            return 84.0/305.9;
                        } else {
                            return 109.0/218.0;
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 7653.5f ) {
                            return 188.0/190.0;
                        } else {
                            if ( cl->stats.glue <= 13.5f ) {
                                return 279.0/212.0;
                            } else {
                                return 241.0/102.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.rdb1_last_touched_diff <= 48514.5f ) {
                        if ( cl->size() <= 10.5f ) {
                            if ( cl->stats.num_overlap_literals_rel <= 0.100251868367f ) {
                                if ( cl->stats.size_rel <= 0.113245472312f ) {
                                    return 12.0/403.9;
                                } else {
                                    return 48.0/633.9;
                                }
                            } else {
                                return 62.0/517.9;
                            }
                        } else {
                            if ( cl->stats.rdb1_used_for_uip_creation <= 3.5f ) {
                                if ( cl->stats.antecedents_glue_long_reds_var <= 1.01999998093f ) {
                                    if ( rdb0_last_touched_diff <= 7336.5f ) {
                                        if ( cl->stats.sum_uip1_used <= 52.5f ) {
                                            return 75.0/337.9;
                                        } else {
                                            return 32.0/459.9;
                                        }
                                    } else {
                                        return 76.0/307.9;
                                    }
                                } else {
                                    if ( rdb_rel_used_for_uip_creation <= 0.5f ) {
                                        return 115.0/272.0;
                                    } else {
                                        return 81.0/409.9;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 37.5f ) {
                                    return 73.0/351.9;
                                } else {
                                    if ( cl->stats.sum_uip1_used <= 327.0f ) {
                                        if ( cl->stats.glue_rel_queue <= 1.06118059158f ) {
                                            return 30.0/635.9;
                                        } else {
                                            return 6.0/457.9;
                                        }
                                    } else {
                                        return 3.0/617.9;
                                    }
                                }
                            }
                        }
                    } else {
                        return 109.0/262.0;
                    }
                }
            } else {
                if ( cl->stats.dump_number <= 15.5f ) {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 21137.0f ) {
                        if ( cl->stats.antec_num_total_lits_rel <= 0.818667471409f ) {
                            return 273.0/98.0;
                        } else {
                            if ( cl->stats.antecedents_glue_long_reds_var <= 8.64710426331f ) {
                                return 175.0/82.0;
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.04883146286f ) {
                                    return 204.0/62.0;
                                } else {
                                    return 297.0/2.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.glue <= 9.5f ) {
                            return 64.0/365.9;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 6214.5f ) {
                                return 55.0/292.0;
                            } else {
                                return 191.0/250.0;
                            }
                        }
                    }
                } else {
                    if ( cl->stats.num_overlap_literals <= 108.0f ) {
                        return 52.0/335.9;
                    } else {
                        return 100.0/315.9;
                    }
                }
            }
        } else {
            if ( rdb0_last_touched_diff <= 32072.0f ) {
                if ( cl->stats.glue_rel_long <= 1.05155277252f ) {
                    if ( cl->stats.num_total_lits_antecedents <= 161.5f ) {
                        if ( rdb0_last_touched_diff <= 11429.0f ) {
                            if ( cl->stats.sum_delta_confl_uip1_used <= 403717.5f ) {
                                return 104.0/333.9;
                            } else {
                                return 39.0/317.9;
                            }
                        } else {
                            if ( cl->size() <= 9.5f ) {
                                return 154.0/421.9;
                            } else {
                                if ( cl->stats.glue <= 8.5f ) {
                                    return 213.0/294.0;
                                } else {
                                    if ( cl->stats.dump_number <= 3.5f ) {
                                        if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                            return 138.0/118.0;
                                        } else {
                                            return 253.0/116.0;
                                        }
                                    } else {
                                        return 132.0/294.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.rdb1_last_touched_diff <= 13301.0f ) {
                            if ( rdb0_act_ranking_top_10 <= 2.5f ) {
                                return 154.0/178.0;
                            } else {
                                return 173.0/86.0;
                            }
                        } else {
                            return 253.0/72.0;
                        }
                    }
                } else {
                    if ( cl->stats.sum_delta_confl_uip1_used <= 31017.0f ) {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 2.04132223129f ) {
                            return 294.0/236.0;
                        } else {
                            if ( cl->stats.size_rel <= 1.11672496796f ) {
                                if ( cl->stats.rdb1_last_touched_diff <= 8687.0f ) {
                                    return 272.0/76.0;
                                } else {
                                    if ( rdb0_last_touched_diff <= 25633.0f ) {
                                        return 267.0/36.0;
                                    } else {
                                        return 196.0/44.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 1.39035582542f ) {
                                    if ( cl->stats.size_rel <= 1.6255967617f ) {
                                        return 222.0/12.0;
                                    } else {
                                        return 188.0/34.0;
                                    }
                                } else {
                                    return 237.0/8.0;
                                }
                            }
                        }
                    } else {
                        if ( cl->stats.antecedents_glue_long_reds_var <= 0.00305487425067f ) {
                            return 97.0/437.9;
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 8946.5f ) {
                                return 110.0/296.0;
                            } else {
                                return 130.0/198.0;
                            }
                        }
                    }
                }
            } else {
                if ( cl->stats.sum_delta_confl_uip1_used <= 42030.5f ) {
                    if ( cl->stats.num_overlap_literals <= 34.5f ) {
                        if ( cl->size() <= 9.5f ) {
                            if ( cl->stats.rdb1_last_touched_diff <= 73897.0f ) {
                                return 134.0/158.0;
                            } else {
                                return 153.0/124.0;
                            }
                        } else {
                            if ( cl->stats.rdb1_last_touched_diff <= 106026.0f ) {
                                if ( cl->stats.num_antecedents_rel <= 0.131420612335f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 4.11111116409f ) {
                                        return 225.0/80.0;
                                    } else {
                                        return 211.0/20.0;
                                    }
                                } else {
                                    if ( cl->stats.num_overlap_literals <= 16.5f ) {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 3.5f ) {
                                            return 190.0/60.0;
                                        } else {
                                            return 189.0/160.0;
                                        }
                                    } else {
                                        if ( cl->stats.size_rel <= 0.77776736021f ) {
                                            return 188.0/132.0;
                                        } else {
                                            if ( cl->stats.size_rel <= 1.1204059124f ) {
                                                return 204.0/34.0;
                                            } else {
                                                return 186.0/48.0;
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_delta_confl_uip1_used <= 3142.5f ) {
                                    if ( cl->stats.dump_number <= 42.5f ) {
                                        if ( cl->size() <= 14.5f ) {
                                            return 250.0/58.0;
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 15.5f ) {
                                                if ( cl->stats.num_antecedents_rel <= 0.180852741003f ) {
                                                    return 243.0/20.0;
                                                } else {
                                                    return 176.0/48.0;
                                                }
                                            } else {
                                                if ( cl->stats.num_overlap_literals <= 27.5f ) {
                                                    if ( rdb0_act_ranking_top_10 <= 7.5f ) {
                                                        return 215.0/18.0;
                                                    } else {
                                                        return 209.0/4.0;
                                                    }
                                                } else {
                                                    return 223.0/40.0;
                                                }
                                            }
                                        }
                                    } else {
                                        return 267.0/70.0;
                                    }
                                } else {
                                    return 188.0/80.0;
                                }
                            }
                        }
                    } else {
                        if ( rdb0_act_ranking_top_10 <= 6.5f ) {
                            if ( cl->stats.glue_rel_long <= 1.06613469124f ) {
                                if ( cl->stats.antec_num_total_lits_rel <= 1.13403296471f ) {
                                    if ( cl->stats.sum_delta_confl_uip1_used <= 9.5f ) {
                                        if ( cl->stats.antecedents_glue_long_reds_var <= 8.46875f ) {
                                            if ( cl->stats.glue_rel_long <= 0.928720593452f ) {
                                                return 329.0/48.0;
                                            } else {
                                                return 227.0/68.0;
                                            }
                                        } else {
                                            if ( cl->stats.num_overlap_literals <= 87.5f ) {
                                                return 248.0/40.0;
                                            } else {
                                                return 319.0/10.0;
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.num_total_lits_antecedents <= 111.0f ) {
                                            return 214.0/112.0;
                                        } else {
                                            return 326.0/82.0;
                                        }
                                    }
                                } else {
                                    if ( cl->stats.num_total_lits_antecedents <= 632.5f ) {
                                        if ( cl->stats.num_antecedents_rel <= 1.77023530006f ) {
                                            if ( cl->stats.num_overlap_literals_rel <= 1.5139465332f ) {
                                                return 259.0/20.0;
                                            } else {
                                                return 259.0/48.0;
                                            }
                                        } else {
                                            return 204.0/60.0;
                                        }
                                    } else {
                                        return 374.0/30.0;
                                    }
                                }
                            } else {
                                if ( cl->stats.sum_uip1_used <= 2.5f ) {
                                    if ( cl->stats.sum_uip1_used <= 0.5f ) {
                                        return 378.0/50.0;
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.58591532707f ) {
                                            if ( cl->stats.rdb1_last_touched_diff <= 34615.5f ) {
                                                return 378.0/46.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 3.669921875f ) {
                                                    return 358.0/38.0;
                                                } else {
                                                    if ( cl->size() <= 31.5f ) {
                                                        if ( cl->stats.num_overlap_literals_rel <= 1.91427493095f ) {
                                                            if ( cl->stats.num_total_lits_antecedents <= 163.5f ) {
                                                                if ( cl->stats.size_rel <= 0.885377287865f ) {
                                                                    return 241.0/22.0;
                                                                } else {
                                                                    return 222.0/6.0;
                                                                }
                                                            } else {
                                                                return 1;
                                                            }
                                                        } else {
                                                            return 217.0/22.0;
                                                        }
                                                    } else {
                                                        if ( cl->stats.antecedents_glue_long_reds_var <= 174.265472412f ) {
                                                            if ( rdb0_last_touched_diff <= 79671.0f ) {
                                                                return 311.0/12.0;
                                                            } else {
                                                                return 1;
                                                            }
                                                        } else {
                                                            return 266.0/18.0;
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            return 358.0/44.0;
                                        }
                                    }
                                } else {
                                    return 373.0/64.0;
                                }
                            }
                        } else {
                            if ( rdb0_act_ranking_top_10 <= 8.5f ) {
                                if ( cl->stats.glue_rel_long <= 0.927182793617f ) {
                                    return 300.0/166.0;
                                } else {
                                    if ( cl->stats.rdb1_last_touched_diff <= 279084.0f ) {
                                        if ( cl->stats.num_overlap_literals <= 82.5f ) {
                                            return 335.0/94.0;
                                        } else {
                                            if ( cl->stats.rdb1_act_ranking_top_10 <= 7.5f ) {
                                                if ( cl->stats.glue <= 17.5f ) {
                                                    return 205.0/60.0;
                                                } else {
                                                    if ( rdb0_last_touched_diff <= 88705.0f ) {
                                                        return 185.0/42.0;
                                                    } else {
                                                        return 188.0/18.0;
                                                    }
                                                }
                                            } else {
                                                if ( cl->stats.sum_delta_confl_uip1_used <= 2.5f ) {
                                                    return 233.0/16.0;
                                                } else {
                                                    return 202.0/34.0;
                                                }
                                            }
                                        }
                                    } else {
                                        if ( cl->stats.glue_rel_queue <= 1.24375367165f ) {
                                            return 267.0/112.0;
                                        } else {
                                            return 184.0/48.0;
                                        }
                                    }
                                }
                            } else {
                                if ( cl->stats.glue_rel_queue <= 0.951620459557f ) {
                                    return 318.0/78.0;
                                } else {
                                    if ( cl->stats.num_antecedents_rel <= 0.543456196785f ) {
                                        if ( cl->stats.antec_num_total_lits_rel <= 0.597431540489f ) {
                                            return 336.0/36.0;
                                        } else {
                                            return 244.0/54.0;
                                        }
                                    } else {
                                        if ( cl->stats.sum_delta_confl_uip1_used <= 19.5f ) {
                                            if ( cl->stats.antecedents_glue_long_reds_var <= 3.99362802505f ) {
                                                return 232.0/24.0;
                                            } else {
                                                if ( cl->stats.antecedents_glue_long_reds_var <= 42.6391906738f ) {
                                                    if ( cl->stats.glue_rel_queue <= 1.21426773071f ) {
                                                        return 235.0/20.0;
                                                    } else {
                                                        return 304.0/8.0;
                                                    }
                                                } else {
                                                    return 275.0/6.0;
                                                }
                                            }
                                        } else {
                                            return 193.0/30.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( cl->stats.sum_uip1_used <= 19.5f ) {
                        if ( rdb0_last_touched_diff <= 74826.0f ) {
                            if ( cl->stats.size_rel <= 0.81614869833f ) {
                                if ( cl->stats.dump_number <= 10.5f ) {
                                    return 109.0/206.0;
                                } else {
                                    return 225.0/210.0;
                                }
                            } else {
                                return 315.0/174.0;
                            }
                        } else {
                            if ( cl->stats.glue <= 11.5f ) {
                                if ( cl->stats.size_rel <= 0.82690089941f ) {
                                    if ( cl->stats.size_rel <= 0.419527173042f ) {
                                        return 171.0/80.0;
                                    } else {
                                        return 256.0/182.0;
                                    }
                                } else {
                                    return 215.0/74.0;
                                }
                            } else {
                                if ( cl->stats.num_overlap_literals <= 67.0f ) {
                                    if ( cl->stats.antecedents_glue_long_reds_var <= 0.67708337307f ) {
                                        return 171.0/70.0;
                                    } else {
                                        return 203.0/62.0;
                                    }
                                } else {
                                    if ( cl->stats.glue_rel_queue <= 1.07960557938f ) {
                                        return 189.0/36.0;
                                    } else {
                                        return 183.0/38.0;
                                    }
                                }
                            }
                        }
                    } else {
                        if ( rdb0_last_touched_diff <= 121513.5f ) {
                            if ( cl->stats.num_overlap_literals <= 12.5f ) {
                                return 79.0/347.9;
                            } else {
                                if ( cl->stats.sum_uip1_used <= 48.5f ) {
                                    return 147.0/220.0;
                                } else {
                                    return 103.0/244.0;
                                }
                            }
                        } else {
                            return 243.0/290.0;
                        }
                    }
                }
            }
        }
    }
}

static bool should_keep_long_conf3_cluster0(
    const CMSat::Clause* cl
    , const uint64_t sumConflicts
    , const uint32_t rdb0_last_touched_diff
    , const uint32_t rdb0_act_ranking
    , const uint32_t rdb0_act_ranking_top_10
) {
    int votes = 0;
    votes += estimator_should_keep_long_conf3_cluster0_0(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_1(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_2(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_3(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_4(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_5(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_6(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_7(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_8(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    votes += estimator_should_keep_long_conf3_cluster0_9(
    cl
    , sumConflicts
    , rdb0_last_touched_diff
    , rdb0_act_ranking
    , rdb0_act_ranking_top_10
    ) < 1.0;
    return votes >= 5;
}
}
