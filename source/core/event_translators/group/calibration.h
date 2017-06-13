#pragma once

class Calibration{
    public:
        // Start calibration machine state 
        virtual void start_calibration(bool reset_calibration) = 0;
        // Save data for calibrate adn return true when calibration proccess finish
        // If we need more data returns false
        virtual bool save_calibrate_data(int64_t value) = 0;
        // Aply the transformation from user_value to game value
        virtual int64_t get_game_value(int64_t user_value) = 0;
        virtual bool is_calibrating() = 0;
        virtual std::shared_ptr<Calibration> clone() = 0;
        virtual ~Calibration(){};
};

// Implements linear calibration: game_values = a * user_value + b
// 
class LinearCalibration : public Calibration {
    typedef enum {
        CALIBRATE_SLEEP,
        CALIBRATE_WAITING_MY_INPUT_0,
        CALIBRATE_WAITING_GAME_INPUT_0,
        CALIBRATE_WAITING_MY_INPUT_1,
        CALIBRATE_WAITING_GAME_INPUT_1,
    }StatusCalibration;
    public:
    LinearCalibration(): a(1.0), b(0.0),status_calibration(CALIBRATE_SLEEP) {};
    LinearCalibration(float a, float b): a(a), b(b), status_calibration(CALIBRATE_SLEEP){};
    virtual ~LinearCalibration(){};
    void start_calibration(bool reset_calibration) {
        status_calibration = CALIBRATE_WAITING_MY_INPUT_0;
        // reset the last calibration
        if(reset_calibration){
            a = 1.0;
            b = 0.0;
        }
        std::cout << "Start Linear Calibration: " 
            << (reset_calibration ? "reset": "composing with last calibration") << std::endl;
    };
    // Save data for calibrate and return true when calibration proccess finish
    // If we need more data returns false
    virtual bool save_calibrate_data(int64_t value){
        switch(status_calibration){
            case CALIBRATE_WAITING_MY_INPUT_0:
                user_values[0] = get_game_value(value);
                status_calibration = CALIBRATE_WAITING_GAME_INPUT_0;
                break;
            case CALIBRATE_WAITING_GAME_INPUT_0:
                // now calibrate
                game_values[0] = get_game_value(value);
                status_calibration = CALIBRATE_WAITING_MY_INPUT_1;
                break;
            case CALIBRATE_WAITING_MY_INPUT_1:
                user_values[1] = get_game_value(value);
                //TODO save my_value;
                status_calibration = CALIBRATE_WAITING_GAME_INPUT_1;
                break;
            case CALIBRATE_WAITING_GAME_INPUT_1:
                // now calibrate
                game_values[1] = get_game_value(value);
                calibrate();
                status_calibration = CALIBRATE_SLEEP;
                return false;
                break;
            default:
                std::cout << __PRETTY_FUNCTION__ 
                    <<"ERROR: unspected state. Stopping calibration " << status_calibration 
                    << std::endl;
                status_calibration = CALIBRATE_SLEEP;
        }
        return true;
    }
    // Aply the transformation from user_value to game value
    virtual int64_t get_game_value(int64_t user_value){
        int64_t result = (user_value * a) + b;
        // check saturation
        if(result > ABS_RANGE){
            result = ABS_RANGE;
        }else if (result < -ABS_RANGE){
            result = -ABS_RANGE;
        }
        return result;
    }
    virtual bool is_calibrating(){
        return status_calibration != CALIBRATE_SLEEP;
    }

    protected:
    float a;
    float b;
    int64_t game_values [2];
    int64_t user_values [2];
    StatusCalibration status_calibration;
    void calibrate(){
        // We have to resolve this system of ecuations
        // game_values[0] = a * user_values[0] + b;
        // game_values[1] = a * user_values[1] + b;
        float tmp_a = ((float) (game_values[0] - game_values[1])) / ( (float) (user_values[0] -  user_values[1]));
        float tmp_b = ((float) (user_values[0]*game_values[1] - game_values[0]*user_values[1])) / ((float)(user_values[0] - user_values[1]));
        // Now we adjust to compose with last calibration.
        a = tmp_a * a;
        b = tmp_a*b + tmp_b;
        std::cout << "Calibration Success. Values are: " 
//            << " user0: " << user_values[0]
//            << " user1: " << user_values[1]
//            << " game0: " << game_values[0]
//            << " game1: " << game_values[1]
            << " a: " << a
            << " b: " << b
                  << std::endl;
    };
    virtual std::shared_ptr<Calibration> clone(){
        return  std::make_shared<LinearCalibration>(a, b);
    }
};
