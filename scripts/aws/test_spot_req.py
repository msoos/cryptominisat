#!/usr/bin/env python
# -*- coding: utf-8 -*-

import RequestSpotClient

#create spot instance requests
spot_create = RequestSpotClient.RequestSpotClient(False)
spot_create.create_spots()
