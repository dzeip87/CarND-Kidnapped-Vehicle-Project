#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

#define NUM_PARTICLES 12

static default_random_engine gen;


void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
    
    normal_distribution<double> dist_x(x, std[0]);
    normal_distribution<double> dist_y(y, std[1]);
    normal_distribution<double> dist_theta(theta, std[2]);
    particles.resize(NUM_PARTICLES);
    weights.resize(NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; i++){
        particles[i].id = i;
        particles[i].x = dist_x(gen);
        particles[i].y = dist_y(gen);
        particles[i].theta = dist_theta(gen);
        particles[i].weight = 1.0 / NUM_PARTICLES;
    }   
    is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
    
    normal_distribution<double> dist_x(0.0, std_pos[0]);
    normal_distribution<double> dist_y(0.0, std_pos[1]);
    normal_distribution<double> dist_theta(0.0, std_pos[2]);

    for (int i = 0; i < NUM_PARTICLES; i++){
        if (fabs(yaw_rate) < 0.000001){
            particles[i].x += velocity * delta_t * cos(particles[i].theta);
            particles[i].y += velocity * delta_t * sin(particles[i].theta);
        } else{
            double theta_new = particles[i].theta + yaw_rate * delta_t;
            particles[i].x += (velocity/yaw_rate) * (sin(theta_new) - sin(particles[i].theta));
            particles[i].y += (velocity/yaw_rate) * (-cos(theta_new) + cos(particles[i].theta));
            particles[i].theta = theta_new;
        }
        // Add random Gaussian noise
        particles[i].x += dist_x(gen);
        particles[i].y += dist_y(gen);
        particles[i].theta += dist_theta(gen);
    }

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

    double dx = 0.0;
    double dy = 0.0;
    double sum_weights = 0.0;
    
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        double weight_factor = 0.0;
        
        for (int j = 0; j < observations.size(); j++)
        {            

            LandmarkObs observation;

            // transform to map coordinates
            observation.id = observations[j].id;
            observation.x = particles[i].x + (observations[j].x * cos(particles[i].theta)) - (observations[j].y * sin(particles[i].theta));
            observation.y = particles[i].y + (observations[j].x * sin(particles[i].theta)) + (observations[j].y * cos(particles[i].theta));
            
            bool out_of_reach = true;
            Map::single_landmark_s nearest_landmark;
            
            double nearest_distance = 10000.0;             // random big number for initialization purposes
            
            for (int k = 0; k < map_landmarks.landmark_list.size(); k++) {

                Map::single_landmark_s next_landmark = map_landmarks.landmark_list[k];

                double distance = dist(next_landmark.x_f, next_landmark.y_f, observation.x, observation.y);
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_landmark = next_landmark;

                    if (distance < sensor_range){
                        dx = observation.x-nearest_landmark.x_f;
                        dy = observation.y-nearest_landmark.y_f;
                        out_of_reach = false;
                    }
                }
            }

            //calculate new weight factor
            if (out_of_reach == false)
            {
                weight_factor += (dx * dx) / (std_landmark[0] * std_landmark[0]) + (dy * dy) / (std_landmark[1] * std_landmark[1]);
            } 
            else
            {
                weight_factor += 1000;   // penalty for out of reach values
            }
        }
        //calculate new weights
        particles[i].weight = exp(-0.5 * weight_factor);
        sum_weights += particles[i].weight;
    }
    
    //normalize the weights
    for (int i = 0; i < NUM_PARTICLES; i++){
        particles[i].weight /= sum_weights * 2 * M_PI * std_landmark[0] * std_landmark[1];
        weights[i] = particles[i].weight;
    }   
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
    
    discrete_distribution<> dist_particles(weights.begin(), weights.end());
    vector<Particle> new_particle;
    new_particle.resize(NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        new_particle[i] = particles[dist_particles(gen)];
    }
    particles = new_particle;
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}