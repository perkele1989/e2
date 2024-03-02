import concurrent.futures

class Job():
    def __init__(self, name:str):
        self.name = name

def iterable_func(job:Job):
    yield "Foo"
    yield "Bar"

    yield "Baz"

def execute_job(job:Job):
    print("BEGIN {}:".format(job.name))
    for x in iterable_func(job):
        print("From {}: {}".format(job.name, x))
    print("END {}".format(job.name))



def main():

    # Create ten jobs, call them job0 to job9
    jobs:list[Job] = []
    for i in range(0, 4):
        jobs.append(Job("job{}".format(i)))

    with concurrent.futures.ThreadPoolExecutor(max_workers=14) as executor:
        futures = []
        for job in jobs:
            #print("Executing job {}..".format(job.name))
            futures.append(executor.submit(execute_job, job=job))
        for x in concurrent.futures.as_completed(futures):
            print("x")

if __name__ == "__main__":
    main()