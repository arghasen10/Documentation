import numpy as np
import pandas as pd
import pickle
import socketserver
import socket
import sys
import os
import struct
import json
from collections import defaultdict, Counter
from time import sleep
import traceback as tb

from util.nestedObject import getObj
from util import cprint

from abr import BOLA

from abr.PowerAbr.utils import get_working_data, set_pandas_display
from abr.PowerAbr.constants import hlf_cols_types, working_columns
from abr.PowerAbr.simulate import convert_to_hlf_data_point, get_label_encoded_data

# import abr.PowerAbr.rnn.Quality as rnnQuality
# import abr.PowerAbr.rnn.Buffer as rnnBuffer
import abr.PowerAbr.rnn.RNN as RNN


BYTES_IN_MB = 1000000.0

BUFFER_ACTION = [-2, -1, 0, +1, +2]

net_type_enc_loc   = 'pretrainEnDashModel/net_type_enc.pkl'
data_state_enc_loc = 'pretrainEnDashModel/data_state_enc.pkl'
data_act_enc_loc   = 'pretrainEnDashModel/data_act_enc.pkl'
save_model_loc     = 'pretrainEnDashModel/rf_model.sav'
scale_loc          = 'pretrainEnDashModel/rf_model_scaler.pkl'

# kgp_data_loc       = 'pretrainEnDashModel/kgp_tpt_data.csv'
# kol_data_loc       = 'pretrainEnDashModel/kol_tpt_data.csv'
# hlf_data_loc       = 'pretrainEnDashModel/hlf_data.csv'
# lab_enc_data_loc   = 'pretrainEnDashModel/lab_enc_data.csv'
# performance_loc    = 'pretrainEnDashModel/rf_model_performance.csv'
# feature_imp_loc    = 'pretrainEnDashModel/rf_model_feature_imp.csv'


BUFFER_ACTION = [-2, -1, 0, +1, +2]

class AbrEnDash:
    def __init__(self, vidLen):
        self.model     = pickle.load(open(save_model_loc, 'rb'))
        self.lab_enc_1 = pickle.load(open(net_type_enc_loc, 'rb'))
        self.lab_enc_2 = pickle.load(open(data_state_enc_loc, 'rb'))
        self.lab_enc_3 = pickle.load(open(data_act_enc_loc, 'rb'))
        self.scale     = pickle.load(open(scale_loc, 'rb'))

        self.modelPath = "./abr/PowerAbr/ResModel/"

        readOnly = True
        if 'LOW_POW_ENV_BUFFER_READ_ONLY' in os.environ:
            readOnly = int(os.environ['LOW_POW_ENV_BUFFER_READ_ONLY']) != 0
        #initiate buffer learner
#         self._vPensieveBufferLearner = None if not self._vModelPath  else rnnBuffer.getPensiveLearner(BUFFER_ACTION, summary_dir = self._vModelPath, readOnly=readOnly)
        self.pensieveBufferLearner = None if not self.modelPath  else RNN.PensiveLearner(BUFFER_ACTION, summary_dir = os.path.join(self.modelPath, "rnnBuffer"), readOnly=True)

        qualityActions = [i for i in range(vidLen)]
        #initiate Quality learner
#         self.pensieveQualityLearner = None if not self.modelPath  else rnnQuality.getPensiveLearner(qualityActions, summary_dir = self.modelPath, readOnly=True)
        self.pensieveQualityLearner = None if not self.modelPath  else RNN.PensiveLearner(qualityActions, summary_dir = os.path.join(self.modelPath, "rnnQuality"), readOnly=True)


    def predict_throughput(self, curr_data):
        if len(curr_data) != 20:
            raise Exception("Data len have to be 20")
        assert type(curr_data) == pd.DataFrame
        curr_data = curr_data[working_columns]
        curr_data.rename(columns={'Throughput tcp':'throughput'}, inplace=True)
        hlf_data = convert_to_hlf_data_point(curr_data)
        enc_data = get_label_encoded_data(hlf_data, self.lab_enc_1, self.lab_enc_2, self.lab_enc_3)
        scaled_data = self.scale.transform(enc_data.values)
        return self.model.predict(scaled_data)[0]

    def getNextQuality(self, vidInfo, agent, agentState):
        nextChunkSizes = [agent.getChunkSize(i, agent.segCount) for i, _ in enumerate(vidInfo.bitrates)]
        lastQuality = agent.requests[-1]["qualityIndex"]
        lastBitrate = vidInfo.bitrates[lastQuality]/ vidInfo.bitrates[-1]
        lastThroughput = agent.requests[-1]["throughput"]
        lastTimeTaken = agent.requests[-1]["timeSpent"]

        bufferUpto = agent.bufferUpto
        playBackTime = agent.playBackTime
        maxPlayerBufferLen = agent.maxPlayerBufferLen

        bufferLeft = (maxPlayerBufferLen - (bufferUpto - playBackTime))

        state = (lastBitrate, bufferLeft, lastThroughput, lastTimeTaken, nextChunkSizes)

        qualityState = agentState.getattr("qualityState", None)
        if qualityState is None:
            qualityState = getObj(s_batch = [])
        res, error = self.pensieveQualityLearner.getNextAction(qualityState, state)
        if res is not None:
            ql, qualityState = res
            cprint.orange("!!PREDICTED QUALITY!!!", ql)
        else:
            retObj = BOLA.getNextQuality(vidInfo, agent) ##proxy
            ql = retObj.repId

        return ql, qualityState, error

    def getAdjustBuffer(self, vidInfo, agent, agentState, predictedThroughput, maxPlayerBufferLen):
        nextChunkSizes = [agent.getChunkSize(i, agent.segCount) for i, _ in enumerate(vidInfo.bitrates)]
        lastQuality = agent.requests[-1]["qualityIndex"]
        lastBitrate = vidInfo.bitrates[lastQuality]/ vidInfo.bitrates[-1]
        lastThroughput = agent.requests[-1]["throughput"]
        lastTimeTaken = agent.requests[-1]["timeSpent"]

        bufferUpto = agent.bufferUpto
        playBackTime = agent.playBackTime
        bufferLeft = (maxPlayerBufferLen - (bufferUpto - playBackTime))

        state = (lastBitrate, bufferLeft, lastThroughput, lastTimeTaken, nextChunkSizes, predictedThroughput)

        bufferState = agentState.getattr("bufferState", None)
        if bufferState is None:
            bufferState = getObj(s_batch = [])
        res, error = self.pensieveBufferLearner.getNextAction(bufferState, state)
        if res is not None:
            ba, bufferState = res
            cprint.orange("!!PREDICTED BUFFER_ACTION!!!", ba)
            return ba, bufferState, error

        return 0, bufferState, error

    def getNextAction(self, vidInfo, agent, agentState):

        ql, qualityState, qualityError = self.getNextQuality(vidInfo, agent, agentState)
        state = getObj(
            qualityState = qualityState,
            )
        errors = getObj(
            qualityError = qualityError,
            )
        retObj = getObj(
            sleepTime = -1, \
            repId = ql, \
            abrState = state, \
            errors = errors, \
            )

        p = {x[0]:x[1:] for x in agent.cellDatas}
        currData = None
        try:
            currData = pd.DataFrame(p)
        except Exception as e:
            cprint.orange("pd", e)
            return retObj
        if len(currData) <= 0:
            return retObj

        dataOffset = 0
        predictedThroughput = -1
        ba = 0
        bufferStates = []
        bufferErrors = []

        currData["throughput"] /= 8

        nextBuf = agent.maxPlayerBufferLen
        segDur = agent.segDur

        #this is another test. might not right but have to to take the risk
        while dataOffset + 20 <= len(currData):
            tmpData = currData[dataOffset : dataOffset + 20]
            cprint.blueS()
            try:
                predictedThroughput = self.predict_throughput(tmpData)

                baTmp, bufferState, bufferError = self.getAdjustBuffer(vidInfo, agent, agentState, predictedThroughput, nextBuf)

                bufferStates.append(bufferState)
                bufferErrors.append(bufferError)

                nextBuf += baTmp * segDur
                if nextBuf < segDur:
                    nextBuf = segDur
                if nextBuf > 100*1000:
                    nextBuf = 100*1000

                ba += baTmp

            except Exception as e:
                track = tb.format_exc()
                throughputError = track
                errors.throughputError = throughputError
                retObj.errors = errors
                cprint.red(track)

            cprint.reset()
            dataOffset += 20
            cprint.orange("!!!PREDICTED THROUGHPUT!!!", predictedThroughput)

#         assert dataOffset <= 20
        if predictedThroughput < 0:
            return retObj

        state.bufferState = bufferStates
        errors.bufferError = bufferErrors

        retObj.adjustBuffer = ba
        retObj.dataProcessed = dataOffset
        retObj.abrState = state
        retObj.errors = errors

        return retObj

SOCKET_PATH = "/tmp/endashsocket"

class AbrHandler(socketserver.BaseRequestHandler):
    def handle(self):
        data = self.request.recv(4, socket.MSG_WAITALL)
        l = struct.unpack("=I", data)[0]
        moreData = self.request.recv(l, socket.MSG_WAITALL)
        vidInfo, agent, agentState = pickle.loads(moreData)

        retObj = getObj(
                repId = len(vidInfo.bitrates),
                abrState = agentState,
                errors = None
                )
        try:
            retObj = self.server.abrObject.getNextAction(vidInfo, agent, agentState)
        except Exception as err:
            track = tb.format_exc()
#             print(track)
            errors = track
            retObj.setattrs(errors=errors)
#             tb.print_exception()
            pass

        sndData = pickle.dumps(retObj)
        l = len(sndData)
        dt = struct.pack("=I", l)
        self.request.send(dt)
        self.request.send(sndData)

class AbrServer(socketserver.UnixStreamServer):
    def __init__(self, abrObject, *arg, **kwarg):
        self.abrObject = abrObject
        super().__init__(*arg, **kwarg)


def startServerInFork(vidLen):
    if os.path.exists(SOCKET_PATH):
        os.remove(SOCKET_PATH)
    pipein, pipeout = os.pipe()
    cprint.green("running Server in fork")
    pid = os.fork()
    if pid == 0:
        cprint.green("Closing unneeded fd")
        for fd in list(range(3, 256)): #it is supposed to be a daemon. So, close all other fd
            if fd in [pipein.real, pipeout.real]:
                continue
            try:
                os.close(fd)
            except OSError as e:
#                 print(e)
                pass
        cprint.green("Closed unneeded fd")
        abrObject = AbrEnDash(vidLen)
        with AbrServer(abrObject, SOCKET_PATH, AbrHandler) as server:
            os.write(pipeout, b"da")
            os.close(pipeout)
            os.close(pipein)
            cprint.green("Server ready in child")
            server.serve_forever()
        sys.exit()

    if pid > 0:
        os.read(pipein, 1)
        os.close(pipeout)
        os.close(pipein)
        cprint.green("Parent stop waiting")

def getNextQualityFromServer(sock, vidInfo, agent, agentState):
    sndData = pickle.dumps((vidInfo, agent, agentState))
    l = len(sndData)
    dt = struct.pack("=I", l)
    sock.send(dt)
    sock.send(sndData)

    dt = sock.recv(4, socket.MSG_WAITALL)
#     print(len(dt))
    l = struct.unpack("=I", dt)[0]
    moreData = sock.recv(l, socket.MSG_WAITALL)
    retObj = pickle.loads(moreData)
    sock.close()

    return retObj

def getNextQuality(vidInfo, agent, minimalState=False):
    #assert not minimalState

    agentState = agent.agentState

    if agentState is None or len(agent.requests) == 0:
        agentState = getObj(
                    lastBitrate = 0,
                    lastTotalRebuf = 0,
                )
        return getObj( \
                abrState = agentState,
                repId = 0,
                errors = None,
                )

    connected = False
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    for x in range(3):
        try:
            sock.connect(SOCKET_PATH)
            connected = True
            break
        except FileNotFoundError:
            pass
        except ConnectionRefusedError:
            pass
        startServerInFork(len(vidInfo.bitrates))

    if not connected:
        return getObj(
                sleepTime = -1, \
                repId = 0, \
                abrState = agentState, \
                errors = None, \
                )

    cprint.green("results from EnDash")

    retObj = getNextQualityFromServer(sock, vidInfo, agent, agentState)
    return retObj


