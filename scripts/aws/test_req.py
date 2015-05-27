#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requester

#create spot instance requests
spot_create = requester.SpotRequestor()
spot_create.create_spots()
