import serial
import msvcrt
import time
import pandas as pd


if __name__ == '__main__':
    input_buffer = []

    # if __debug__:
    #     current_iteration = 0
    #     max_iterations = 5

    print("Enter port to connect to (eg COM5, COM13, etc)")
    selection = raw_input("PORT:")

    ser = serial.Serial(port=selection, baudrate=921600)
    print("Connected to: " + ser.portstr)

    raw_input("Press enter to begin recording. To stop, press any key.")
    recording = True
    while recording:
        if msvcrt.kbhit():
            recording = False

        bytes_to_read = ser.inWaiting()
        data_string = ser.read(bytes_to_read)
        input_buffer.append(data_string)

        # if __debug__:
        #     if current_iteration == max_iterations:
        #         recording = False
        #     else:
        #         print("Iteration:" + str(current_iteration))
        #         current_iteration += 1

        time.sleep(1)

    print("Stopped recording. Now closing serial port....")
    ser.close()
    print("Done. Beginning CSV creation.")

    raw_str = ''.join(input_buffer)
    recorded_lines = raw_str.split("\r\n")

    parsed_lines = [line.split(',') for line in recorded_lines]
    data_frame = pd.DataFrame(parsed_lines, columns=['pitch (deg)', 'roll (deg)', 'yaw (deg)', 'ax (m/s^2)',
                                                     'ay (m/s^2)', 'az (m/s^2)', 'gx (deg/s)', 'gy (deg/s)',
                                                     'gz (deg/s)'])

    data_frame.to_csv('ahrs_recorded_output.csv')
    print("Finished writing to csv.")




