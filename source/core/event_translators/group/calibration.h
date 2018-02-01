#pragma once

class Calibration{
    public:
        virtual int get_number_samples_needed_to_calibrate() = 0; 
        // Get a string to show information about the sample input.
        virtual std::string get_feedback_msg(int index) = 0;

        // Reset calibration, now the class returns raw input.
        virtual void reset_calibration() = 0;
        // Readjust the calibration
        virtual void calibrate(const std::vector<int64_t>& input_samples) = 0;

        // Aply the transformation from user_value to game value
        virtual int64_t get_game_value(int64_t user_value) = 0;



        virtual std::shared_ptr<Calibration> clone() = 0;
        virtual ~Calibration(){};
};

// Implements linear calibration: game_values = a * user_value + b
// 
class LinearCalibration : public Calibration {
    public:
        LinearCalibration(): a(1.0), b(0.0){};
        LinearCalibration(float a, float b): a(a), b(b){};

        virtual ~LinearCalibration(){};

        virtual int get_number_samples_needed_to_calibrate(){
            return 4;
        } 

        virtual std::string get_feedback_msg(int index){
            switch(index){
                case 0:
                    return "Taking max value of your controller";
                case 1:
                    return "Taking max value in game";
                case 2:
                    return "Taking min value of your controller";
                case 3:
                    return "Taking min value in game";
                default:
                    break;
            }
            return "ERROR: bad value of index. We only take 4 samples.";

        }
        virtual void reset_calibration(){
            a = 1.0;
            b = 0.0;
        }
        virtual void calibrate(const std::vector<int64_t>& input_samples){
            int64_t user_values [2] = {get_game_value(input_samples[0]), get_game_value(input_samples[2])};
            int64_t game_values [2] = {input_samples[1], input_samples[3]};
            
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
        virtual std::shared_ptr<Calibration> clone(){
            return  std::make_shared<LinearCalibration>(a, b);
        }

    protected:
        float a;
        float b;
};
