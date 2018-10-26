import sys
import os

from flask import Flask, render_template, request, redirect, Response, jsonify
from time import sleep
import json
import unicodedata
import math
import subprocess

app = Flask(__name__)

STATE0 = 2130706432  # 0x7F000000
STATE1 = 32512  # 0x00007F00
PAULIX = 6
PAULIY = 7
PAULIZ = 8
SQRTNOT = 9
HADAMARD = 10
CNOT1 = 11
CNOT2 = 12
TOFFOLI1 = 13
TOFFOLI2 = 14
TOFFOLI3 = 15

CPATH = "c"

gateParse = {
    'pauliX': 6,
    'pauliY': 7,
    'pauliZ': 8,
    'sqrtNot': 9,
    'hadamard': 10,
    'spacerCNOT': 11, #control qubit of CNOT
    'CNOT': 12, #corresponds to row of CONTROLLED bit
    'spacer1CCNOT': 13, #control qubit 0
    'spacer2CCNOT': 14, #control qubit 1
    'CCNOT': 15, #corresponds to row of CONTROLLED bit
    'bypass': -1,
    'result': -2,
    'input': -3,
}


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/quantum.html')
def desktop():
    return render_template('quantum.html')


@app.route('/quantumMobile.html')
def mobile():
    return render_template('quantumMobile.html')


# HTML:
# 1  2  3  4  5  6  7  8  9
# 10 11 12 13 14 15 16 17 18
# 19 20 21 22 23 24 25 26 27
# 1, 10, and 19 are start states
# PYTHON:
# i0 0  1  2  3  4  5  6  7
# i1 8  9  10 11 12 13 14 15
# i2 16 17 18 19 20 21 22 23
# start states not included

# SUBPROCESS FORMAT:
# args_in = ("PATH_TO_EXECUTABLE/executable", "arg1", "arg2", ..., "argn")
# process = subprocess.Popen(args_in, stdout=subprocess.PIPE)
# process.wait()
# output = process.stdout.read()


@app.route('/receiver', methods=['POST'])
def worker():

    #local function for error checking
    def check_error(error_code):
        if not error_code:
            print("No error")
        else:
            if error_code == -1:
                print("Couldn't write input state -- improper executable args")
            elif error_code == -2:
                print("Couldn't write input state -- File I/O error")
            else:
                print("Unknown error")

    #local function for writing a gate using the c executable
    # path - path to and including executable to call (i.e. ./write_gate)
    # (c, r) - (column, row) of gate being written. row will correspond to gate,
    #          i.e. PauliX = row 6
    # prev - previous gate, i.e. selecting mux value
    def write_gate(path, c, r, prev):
        input_args = (path, str(c), str(r), str(prev))
        print(path + " " + str(c) + " " + str(r) + " " +
                  str(prev))
        p = subprocess.Popen(input_args, stdout=subprocess.PIPE)
        p.wait()
        e = p.stdout.read()
        check_error(e)

    #local function for writing to input stage using c executable
    # path - path to and including executable to call (i.e. ./write_input)
    # s - qubit state represented in 32-bit fixed point notation
    # r - input row
    def write_input(path, s, r):
        input_args = (path, str(s), str(r))
        p = subprocess.Popen(input_args, stdout=subprocess.PIPE)
        p.wait()
        e = p.stdout.read()
        check_error(e)

    #local function for reading output and returning the result using c executable 
    # path - path to and including executable to call (i.e. ./read_output)
    # r - output row 
    def read_output(path, r):
        input_args = (path, str(r))
        p = subprocess.Popen(input_args, stdout=subprocess.PIPE)
        p.wait()
        e = p.stdout.read()

        #different error handling than normal, as we are expecting an output value
        if e == -1:
            print("Couldn't read output state - improper executable arg")
        elif e == -2:
            print("Couldn't read output state - File I/O error")
        else:
            print("Output state: " + str(e))

        return e

    #local function to convert 32-bit number representing qubit state probabilities
    #into four distinct probabilities - alpha.real, alpha.imag, beta.real, beta.imag
    #Format of 32-bit qubit number:
    #| 8 bit alpha real | 8 bit alpha imag | 8 bit beta real | 8 bit beta imag |
    #Each 8-bit value listed above is represented in fixed point where the MSB has
    #a weight of 2^0 and each subsequent bit is a lower power by 1. i.e.
    # 01000000 = 2**-1 = 0.5
    #Finally, function calculates the probability of reading state |0} and returns
    #this value
    def fixed_to_prob(qprob):

        #helper function -- converts 8-bit fixed point to a probability
        #Assumes lower 8 bits of val are the fixed point bits
        def ftp_helper(val):

            if (val & 0b10000000) != 0:
                offset = -1
            else:
                offset = 0

            rv = offset
            for i in range(1, 8):
                rv = rv + ((val >> 7-i) & 1) * 2**-i

            return rv

        if qprob == STATE0:
            state0prob = 1
        else:
            areal = (qprob & 0xff000000) >> 24
            aimag = (qprob & 0x00ff0000) >> 16
            breal = (qprob & 0x0000ff00) >> 8
            bimag = qprob & 0x000000ff
        
            arealprob = ftp_helper(areal)
            aimagprob = ftp_helper(aimag)
            brealprob = ftp_helper(breal)
            bimagprob = ftp_helper(bimag)

            #calculate probability of state 0
            #Probability of measuring |0} is |alpha|^2
            #P = |alpha|^2
            #  = sqrt((alpha.real)^2+(alpha.imag)^2)^2
            #  = alpha.real^2 + alpha.imag^2
            state0prob = arealprob**2 + aimagprob**2

        print("Probability of measuring |0}: " + str(state0prob))

        return state0prob
        

    numItemsPerRow = 9
    # read JSON and reply
    jsData = request.get_json(force=True)
    arg0 = str(jsData[0])
    arg1 = str(jsData[1])
    arg2 = str(jsData[2])
    arg3 = str(jsData[3])
    arg4 = str(jsData[4])
    arg5 = str(jsData[5])
    arg6 = str(jsData[6])
    arg7 = str(jsData[7])

    if ((arg0 == "oneStart") or (arg0 == "zeroStart")):
        startRow = int(filter(str.isdigit, arg1))
        startRow = int(math.floor(startRow / numItemsPerRow))
        c_path = os.path.join(CPATH, "write_input")
        if (arg0 == "oneStart"):
            print(str(STATE1) + " " + str(startRow))
            # call c program with STATE1 and startRow
            write_input(c_path, STATE1, startRow)
            # c_input_args = (c_path, str(STATE1), str(startRow))
            # c_input = subprocess.Popen(c_input_args, stdout=subprocess.PIPE)
            # c_input.wait()
            # e = c_input.stdout.read()
            # check_error(e)

        else:
            # call c program with STATE0 and startRow
            print(str(STATE0) + " " + str(startRow))
            write_input(c_path, STATE0, startRow)

    elif ((arg0 == "output")):
        print("output")
        print("arg1: " + arg1 + " arg2: " + arg2 + " arg3: " + arg3)
        # row = int(filter(str.isdigit, arg1))
        # row = int(math.floor((row - 1) / numItemsPerRow))
        # column = int(filter(str.isdigit, arg1))
        # column = int(math.floor((column - 1) % numItemsPerRow)) - 1
        c_path = os.path.join(CPATH, "read_output")

        gate0 = gateParse[arg1]
        gate1 = gateParse[arg2]
        gate2 = gateParse[arg3]

        if gate0 == -1:
            gate0 = 0
        if gate1 == -1:
            gate1 = 1
        if gate2 == -1:
            gate2 = 2

        out0 = read_output(c_path, gate0)
        out1 = read_output(c_path, gate1)
        out2 = read_output(c_path, gate2)

        prob0 = fixed_to_prob(int(out0))
        prob1 = fixed_to_prob(int(out1))
        prob2 = fixed_to_prob(int(out2))

        return (str(prob0) + "~" + str(prob1) + "~" + str(prob2))
        #return output values if not ==-1 or ==-2


    elif ((arg0 == "remove")):
        row = int(filter(str.isdigit, arg1))
        row = int(math.floor((row - 1) / numItemsPerRow))
        column = int(filter(str.isdigit, arg1))
        column = int(math.floor((column - 1) % numItemsPerRow)) - 1
        c_path = os.path.join(CPATH, "write_gate")

        #If removing a normal gate, arg4-arg7 unused
        #If removing a CNOT gate, arg6-arg7 are unused
        #If removing a CCNOT gate, all args are used
        
        if arg4 == "unused": #removing a normal gate
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]

            #If previous gate is bypass or input
            if prevGate == -1 or prevGate == -3:
                prevGate = row  #Use bypass in row corresponding to qubit

            if nextGate == -1:
                nextGate = row  #Use bypass in row corresponding to qubit

            #replace current gate with bypass
            write_gate(c_path, column, row, prevGate)

            #modify next gate so that selecting mux points to new bypass, but only if
            #next gate is not the output stage
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, row)
            
        elif arg6 == "unused": #removing a CNOT gate
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]
            prevControlGate = gateParse[arg4]
            nextControlGate = gateParse[arg5]
            print("Removing CNOT at col " + str(column) + " and Row " + str(row))
            print("Previous gate: " + arg2 + " Next gate: " + arg3)
            print("Previous control gate: " + arg4 + " Next control gate: " + arg5)

            if prevGate == -1 or prevGate == -3: #bypass/input
                prevGate = row
            if prevControlGate == -1 or prevControlGate == -3: #bypass/input
                prevControlGate = row-1

            if nextGate == -1: #bypass
                nextGate = row
            if nextControlGate == -1: #bypass
                nextControlGate = row-1

            #Set bypass at location of CNOT
            write_gate(c_path, column, row, prevGate)
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, row)

            #Set bypass at location of CNOT control qubit
            write_gate(c_path, column, row-1, prevControlGate)
            if nextControlGate != -2:
                write_gate(c_path, column+1, nextControlGate, row-1)
            
            
        else: #removing a CCNOT gate
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]
            prevGate1 = gateParse[arg4]
            nextGate1 = gateParse[arg5]
            prevGate0 = gateParse[arg6]
            nextGate0 = gateParse[arg7]
            print("Removing CCNOT at col " + str(column) + " and Row " + str(row))
            print("Previous gate: " + arg2 + " Next gate: " + arg3)
            print("Previous control gate 0: " + arg6 + "Next control gate 0: " + arg7)
            print("Previous control gate 1: " + arg4 + "Next control gate 1: " + arg5)

            if prevGate == -1 or prevGate == -3:
                prevGate = row
            if prevGate1 == -1 or prevGate1 == -3:
                prevGate1 = row-1
            if prevGate0 == -1 or prevGate0 == -3:
                prevGate0 = row-2

            if nextGate == -1:
                nextGate = row
            if nextGate1 == -1:
                nextGate1 = row-1
            if nextGate0 == -1:
                nextGate0 = row-2

            #Set bypass at location of CCNOT 
            write_gate(c_path, column, row, prevGate)
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, row)

            #Set bypass for control qubit 1
            write_gate(c_path, column, row-1, prevGate1)
            if nextGate1 != -2:
                write_gate(c_path, column+1, nextGate1, row-1)

            #Set bypass for control qubit 0
            write_gate(c_path, column, row-2, prevGate0)
            if nextGate0 != -2:
                write_gate(c_path, column+1, nextGate0, row-2)


    elif ((arg0 == 'initialize')):
        #Make all bypasses in all 3 input rows connected as well as 
        #initializing input states to |0}
        c_path_wr = os.path.join(CPATH, "write_gate")
        c_path_in = os.path.join(CPATH, "write_input")

        #Inputs
        write_input(c_path_in, STATE0, 0)
        write_input(c_path_in, STATE0, 1)
        write_input(c_path_in, STATE0, 2)

        #Loop to connect all bypasses for each row
        for row in range(0, 3):
            for col in range(0, 8):
                write_gate(c_path_wr, col, row, row)

    else:
        # addedGate, location, prevGate, nextGate
        # prevGate is 0,1,2 if connected to input state
        # nextGate is result if connected to end
        # next/prevGate is bypass if not connected
        # col refer to HTML+VHDL col
        # row refers qbit row
        # gateToAdd is row number of gate in VHDL
        row = int(filter(str.isdigit, arg1))
        row = int(math.floor((row - 1) / numItemsPerRow))
        column = int(filter(str.isdigit, arg1))
        column = int(math.floor((column - 1) % numItemsPerRow)) - 1
        gateToAdd = gateParse[arg0]
        c_path = os.path.join(CPATH, "write_gate")

        #Special cases for CCNOT and CNOT
        if arg0 == 'CNOT':  #Assume control qubit is always (row-1)
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]
            prevControlGate = gateParse[arg4]
            nextControlGate = gateParse[arg5]
            print("CNOT at col " + str(column) + " and Row " + str(row))
            print("Previous gate: " + arg2 + " Next gate: " + arg3)
            print("Previous control gate: " + arg4 + " Next control gate: " +
                  arg5)
            
            if prevGate == -1 or prevGate == -3: #bypass or input
                prevGate = row
            if prevControlGate == -1 or prevControlGate == -3: #bypass or input
                prevControlGate = row-1
            
            if nextGate == -1:
                nextGate = row
            if nextControlGate == -1:
                nextControlGate = row-1

            #Connect CNOT1 (controlled qubit)
            write_gate(c_path, column, gateToAdd, prevGate)
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, gateToAdd)

            #Connect CNOT0 (controlling qubit)
            write_gate(c_path, column, gateToAdd-1, prevControlGate)
            if nextControlGate != -2:
                write_gate(c_path, column+1, nextControlGate, gateToAdd-1)

        elif arg0 == 'CCNOT':  #Assume control qubits are always (row-1) and (row-2)
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]
            prevGate1 = gateParse[arg4]
            nextGate1 = gateParse[arg5]
            prevGate0 = gateParse[arg6]
            nextGate0 = gateParse[arg7]
            print("CCNOT at col " + str(column) + " and Row " + str(row))
            print("Previous gate: " + arg2 + " Next gate: " + arg3)
            print("Previous control gate 0: " + arg6 + "Next control gate 0: " + arg7)
            print("Previous control gate 1: " + arg4 + "Next control gate 1: " + arg5)

            if prevGate == -1 or prevGate == -3:
                prevGate = row
            if prevGate1 == -1 or prevGate1 == -3:
                prevGate1 = row-1
            if prevGate0 == -1 or prevGate0 == -3:
                prevGate0 = row-2

            if nextGate == -1:
                nextGate = row
            if nextGate1 == -1:
                nextGate1 = row-1
            if nextGate0 == -1:
                nextGate0 = row-2

            #Connect CCNOT2 (controlled qubit)
            write_gate(c_path, column, gateToAdd, prevGate)
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, gateToAdd)

            #Connect CCNOT1 (second controlling qubit)
            write_gate(c_path, column, gateToAdd-1, prevGate1)
            if nextGate1 != -2:
                write_gate(c_path, column+1, nextGate1, gateToAdd-1)

            #Connect CCNOT0 (first controlling qubit)
            write_gate(c_path, column, gateToAdd-2, prevGate0)
            if nextGate0 != -2:
                write_gate(c_path, column+1, nextGate0, gateToAdd-2)

        #normal case
        else:
            prevGate = gateParse[arg2]
            nextGate = gateParse[arg3]
            print("Col: " + str(column) + " Row: " + str(row) + " Added: " +
                  arg0 + " VHDL Row: " + str(gateToAdd) + " Prev: " + arg2 +
                  " Next: " + arg3)

            #If previous gate is bypass or input
            if prevGate == -1 or prevGate == -3:
                prevGate = row  #Use bypass in row corresponding to qubit

            if nextGate == -1:
                nextGate = row  #Use bypass in row corresponding to qubit

            #Connect added gate to previous gate
            write_gate(c_path, column, gateToAdd, prevGate)

            #Connect next gate to added gate but only if next gate is not output stage
            if nextGate != -2:
                write_gate(c_path, column+1, nextGate, gateToAdd)
                # c_wr2_args = (c_path, str(column + 1), str(nextGate),
                #               str(gateToAdd))
                # c_wr2 = subprocess.Popen(c_wr2_args, stdout=subprocess.PIPE)
                # c_wr2.wait()

                # print(c_path + " " + str(column + 1) + " " + str(nextGate) +
                #       " " + str(gateToAdd))

                # #error checking
                # e2 = c_wr2.stdout.read()
                # check_error(e2)

    return "Ok"


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=10000)
