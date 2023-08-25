semaphore customers = 0;	// counts number of waiting customers
semaphore chef = 0;			// number of chefs available (0 or 1)
semaphore working = 0;		// can only service one customer at a time
semaphore mutex = 1;		// for mutual exclusion of critical resource, used to prevent num_customers from being changed outside of the thread
int max_customers = n;
int num_customers = 0;		// number of customers

void chef() {
    while(1) {
        // wait for a customer to arrive
        down(customers);

        // get mutex to update num_customers
        down(mutex);
        // decrement the number of customers
        num_customers--;
        // release mutex
        up(mutex);

        // chef is now available to help the customer
        up(chef);

        // help the customer
        service_customer();

        // continue watching dramas until another customer arrives
    }
}

void customer() {
    while(1) {
        // get mutex to update num_customers
        down(mutex);

        if (num_customers < max_customers) {
            // increment number of customers
            num_customers++;

            // tell the chef that a customer is waiting
            up(customers);
            // release the mutex
            up(mutex);
            // wait for the chef to become available
            down(chef);
            // get help from the chef
            get_help();
        } else {
            // release the mutex
            up(mutex);
            // no available seats, leave the bakery
        }
    }
}

// Chef helps customer only if there is a customer to request it
// Recall that working is initialized to 0. Chef can acquire the semaphore when working = 1 (customer gets help)
service_customer() {
    down(working);
}

get_help() {
    // Customer asks for help so now working = 1 (assuming working was 0 before)
    up(working);
}

down(semaphore s) {
    if (s > 0) {
        s -= 1;
    } else {
        // sleep and watch all the dramas;
    }
}

up(semaphore s) {
    if(someone is waiting on s) {
        // wake up
    } else {
        s += 1;
    }
}

